/**
 * 스마트 출석체크 — 중계 서버 (Google Apps Script)
 * ----------------------------------------------------
 * 앱(Cloud Run)과 ESP32가 서로 직접 통신할 수 없으므로,
 * 둘 다 접속 가능한 이 웹앱이 가운데에서 심부름을 한다.
 *
 * 구글 시트 구성 (이 스크립트가 붙어있는 시트):
 *   [학생명단] A:UID  B:이름  C:학년  D:반  E:번호  F:등록일시
 *   [출석기록] A:날짜  B:시각  C:교시  D:UID  E:이름  F:상태(출석/지각)  G:리더기
 *
 * 주고받는 말 (action 파라미터):
 *   ── 앱이 부르는 것 ──
 *   start  : 세션 시작.  mode=attend|register, device, [name, grade, klass, number,
 *            period, lateAfter(HH:MM), timeout(초, 기본 30)]         → JSON
 *   status : 세션 상태/결과 조회. device                              → JSON
 *   cancel : 세션 취소. device                                        → JSON
 *   ── ESP32가 부르는 것 ──
 *   poll   : 할 일 조회. device                → "IDLE" | "ATTEND" | "REGISTER"
 *   tag    : 카드 태그 보고. device, uid       → "OK,이름,상태,시각" 등 한 줄 텍스트
 */

var SHEET_STUDENTS = '학생명단';
var SHEET_LOG = '출석기록';

function doGet(e)  { return route(e); }
function doPost(e) { return route(e); }

function route(e) {
  var p = (e && e.parameter) || {};
  switch (p.action) {
    case 'start':  return startSession(p);
    case 'status': return sessionStatus(p);
    case 'cancel': return cancelSession(p);
    case 'poll':   return devicePoll(p);
    case 'tag':    return deviceTag(p);
    default:       return text('ERR,unknown_action');
  }
}

// ────────────────────────── 앱 쪽 ──────────────────────────

// 출석/등록 세션 시작. 세션은 리더기(device) 1대당 1개, 캐시에 저장(자동 만료)
function startSession(p) {
  if (!p.device || (p.mode !== 'attend' && p.mode !== 'register')) {
    return json({ ok: false, error: 'device와 mode(attend|register)가 필요합니다' });
  }
  if (p.mode === 'register' && !p.name) {
    return json({ ok: false, error: '등록 모드는 name(학생 이름)이 필요합니다' });
  }
  var timeout = Math.min(Math.max(parseInt(p.timeout || '30', 10), 10), 300);
  var sess = {
    mode: p.mode,             // attend | register
    state: 'waiting',         // waiting → done
    device: p.device,
    name: p.name || '',       // 대상 학생 (출석 모드에서 비우면 아무나 태그 가능)
    grade: p.grade || '',
    klass: p.klass || '',
    number: p.number || '',
    period: p.period || '',   // 교시 (예: "1")
    lateAfter: p.lateAfter || '',  // 이 시각(HH:MM) 이후 태그는 지각
    result: null
  };
  CacheService.getScriptCache().put(cacheKey(p.device), JSON.stringify(sess), timeout);
  return json({ ok: true, timeout: timeout });
}

// 앱이 1초마다 물어보는 세션 상태
// state: none(세션 없음/만료) | waiting(태그 대기) | done(결과 있음)
function sessionStatus(p) {
  var sess = loadSession(p.device);
  if (!sess) return json({ state: 'none' });
  return json({ state: sess.state, mode: sess.mode, result: sess.result });
}

function cancelSession(p) {
  CacheService.getScriptCache().remove(cacheKey(p.device));
  return json({ ok: true });
}

// ────────────────────────── ESP32 쪽 ──────────────────────────

// ESP32가 2초마다 물어보는 "할 일" — 한 단어로만 답한다
function devicePoll(p) {
  var sess = loadSession(p.device);
  if (!sess || sess.state !== 'waiting') return text('IDLE');
  return text(sess.mode === 'register' ? 'REGISTER' : 'ATTEND');
}

// ESP32가 카드를 태그했을 때. 시트 기록까지 여기서 처리한다
function deviceTag(p) {
  if (!p.device || !p.uid) return text('ERR,param');
  var lock = LockService.getScriptLock();   // 동시 태그로 시트가 꼬이지 않게
  lock.waitLock(5000);
  try {
    var sess = loadSession(p.device);
    if (!sess || sess.state !== 'waiting') return text('IDLE');

    var uid = String(p.uid).toUpperCase();
    var out = (sess.mode === 'register') ? doRegister(sess, uid) : doAttend(sess, uid);

    // 결과를 세션에 남겨서 앱이 status 폴링으로 가져가게 한다 (60초 유지)
    sess.state = 'done';
    CacheService.getScriptCache().put(cacheKey(p.device), JSON.stringify(sess), 60);
    return text(out);
  } finally {
    lock.releaseLock();
  }
}

// 등록 모드: UID를 학생명단에 저장
function doRegister(sess, uid) {
  var sheet = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  var found = findStudent(uid);
  if (found) {
    sess.result = { ok: false, reason: 'dup', name: found.name };
    return 'DUP,' + found.name;
  }
  sheet.appendRow([uid, sess.name, sess.grade, sess.klass, sess.number, now('yyyy-MM-dd HH:mm:ss')]);
  sess.result = { ok: true, name: sess.name, uid: uid };
  return 'OK_REG,' + sess.name;
}

// 출석 모드: UID로 학생을 찾아 출석기록에 저장 (지각 판정 포함)
function doAttend(sess, uid) {
  var student = findStudent(uid);
  if (!student) {
    sess.result = { ok: false, reason: 'unknown', uid: uid };
    return 'UNKNOWN';
  }
  // 특정 학생을 대상으로 한 세션이면, 다른 학생 카드는 거절
  if (sess.name && sess.name !== student.name) {
    sess.result = { ok: false, reason: 'mismatch', name: student.name };
    return 'MISMATCH,' + student.name;
  }

  var time = now('HH:mm:ss');
  var status = '출석';
  if (sess.lateAfter && time > sess.lateAfter + ':00') status = '지각';

  var sheet = getSheet(SHEET_LOG, ['날짜', '시각', '교시', 'UID', '이름', '상태', '리더기']);
  sheet.appendRow([now('yyyy-MM-dd'), time, sess.period, uid, student.name, status, sess.device]);

  sess.result = { ok: true, name: student.name, status: status, time: time };
  return 'OK,' + student.name + ',' + status + ',' + time;
}

// ────────────────────────── 도우미 ──────────────────────────

function cacheKey(device) { return 'sess_' + device; }

function loadSession(device) {
  if (!device) return null;
  var raw = CacheService.getScriptCache().get(cacheKey(device));
  return raw ? JSON.parse(raw) : null;
}

// UID로 학생명단에서 학생 찾기
function findStudent(uid) {
  var sheet = getSheet(SHEET_STUDENTS, ['UID', '이름', '학년', '반', '번호', '등록일시']);
  var rows = sheet.getDataRange().getValues();
  for (var i = 1; i < rows.length; i++) {
    if (String(rows[i][0]).toUpperCase() === uid) {
      return { name: String(rows[i][1]), grade: rows[i][2], klass: rows[i][3], number: rows[i][4] };
    }
  }
  return null;
}

// 시트가 없으면 머리글과 함께 만들어서 돌려준다
function getSheet(name, headers) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(name);
  if (!sheet) {
    sheet = ss.insertSheet(name);
    sheet.appendRow(headers);
    sheet.setFrozenRows(1);
  }
  return sheet;
}

function now(format) {
  return Utilities.formatDate(new Date(), 'Asia/Seoul', format);
}

function text(s) {
  return ContentService.createTextOutput(s).setMimeType(ContentService.MimeType.TEXT);
}

function json(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(ContentService.MimeType.JSON);
}
