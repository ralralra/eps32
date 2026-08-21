/*
  우산 대여 시스템 — Apps Script (구글 시트 = DB)
  ------------------------------------------------
  시트 탭: users / lockers / umbrellas / rentals / plans
           (commands 탭은 없으면 자동 생성 — ESP32 명령 큐)

  배포: 확장 프로그램 → Apps Script → 배포 → 새 배포 → 웹 앱
        "액세스 권한: 모든 사용자" → /exec URL을 웹앱과 ESP32에 넣기

  브라우저 테스트:
    URL?action=rent&user_id=U001            → 대여 (슬롯 열림 명령까지)
    URL?action=return&user_id=U001          → 반납
    URL?action=status                       → 우산 현황 JSON
    URL?action=cmd                          → ESP32용 명령 읽기(읽으면 비워짐)

  ★ 대여 한 건 = rentals 추가 + umbrellas 갱신 + lockers 슬롯 수 갱신을
    이 스크립트가 "한 번에" 처리 — 시트를 손으로 고치면 정합성이 깨져요!
*/

const TAB = {                    // ★ 실제 시트 탭 이름과 다르면 여기만 고치세요
  users:     "users",
  lockers:   "lockers",
  umbrellas: "umbrellas",
  rentals:   "rentals",
  plans:     "plans",
  commands:  "commands"
};
const DEFAULT_PLAN = "P001";     // 요금제 미지정 시 24시간권

// ───────────────────────── 진입점 ─────────────────────────
function doGet(e) {
  const a = (e.parameter.action || "").toLowerCase();
  const lock = LockService.getScriptLock();   // 동시 요청 충돌 방지
  lock.waitLock(5000);
  try {
    if (a === "rent")   return json(rent(e));
    if (a === "return") return json(returnUmbrella(e));
    if (a === "status") return json(status());
    if (a === "cmd")    return text(popCommand());
    if (a === "report") return json(report(e));
    return json({ ok: false, error: "action을 지정하세요 (rent/return/status/cmd/report)" });
  } catch (err) {
    return json({ ok: false, error: String(err) });
  } finally {
    lock.releaseLock();
  }
}

// ───────────────────────── 액션 ─────────────────────────

// 대여: 빈 우산 배정 → rentals 추가 + umbrellas·lockers 갱신 + 슬롯 열림 명령
function rent(e) {
  const userId = e.parameter.user_id;
  if (!userId) return { ok: false, error: "user_id가 필요해요" };
  if (!findRow(TAB.users, "user_id", userId))
    return { ok: false, error: "등록되지 않은 사용자: " + userId };
  if (findActiveRental(userId))
    return { ok: false, error: "이미 대여 중이에요 — 먼저 반납하세요" };

  // 사용 가능한 우산 찾기 (슬롯 번호가 숫자인 정상 행만)
  const um = sheetTable(TAB.umbrellas);
  const target = um.rows.find(r =>
    r.status === "available" && !isNaN(parseInt(r.slot_no)));
  if (!target) return { ok: false, error: "지금은 빌릴 수 있는 우산이 없어요" };

  const plan = findRow(TAB.plans, "plan_id", e.parameter.plan_id || DEFAULT_PLAN);
  const hours = plan ? Number(plan.hours) : 24;
  const price = plan ? Number(plan.price) : 0;

  const now = new Date();
  const expected = new Date(now.getTime() + hours * 3600 * 1000);
  const rentalId = nextId(TAB.rentals, "rental_id", "R");

  // ① rentals 한 줄 추가
  appendByHeader(TAB.rentals, {
    rental_id: rentalId, user_id: userId,
    umbrella_id: target.umbrella_id, locker_id: target.locker_id,
    slot_no: target.slot_no, plan_hours: hours, plan_price: price,
    rental_time: now, expected_return: expected,
    status: "active", payment_status: "pending"
  });

  // ② umbrellas 갱신
  updateRow(TAB.umbrellas, "umbrella_id", target.umbrella_id, {
    status: "rented", last_user_id: userId, last_check_time: now,
    total_rentals: Number(target.total_rentals || 0) + 1
  });

  // ③ lockers 빈 슬롯 수 갱신
  bumpAvailableSlots(target.locker_id, -1);

  // ④ ESP32에 "슬롯 열어" 명령
  pushCommand("RENT:" + target.slot_no);

  return { ok: true, rental_id: rentalId, umbrella_id: target.umbrella_id,
           slot_no: Number(target.slot_no), expected_return: expected };
}

// 반납: 내 active 대여 찾기 → 원래 슬롯 열림 → rentals·umbrellas·lockers 갱신
function returnUmbrella(e) {
  const userId = e.parameter.user_id;
  if (!userId) return { ok: false, error: "user_id가 필요해요" };

  const rental = findActiveRental(userId);
  if (!rental) return { ok: false, error: "대여 중인 우산이 없어요" };

  const now = new Date();
  const durationMin = Math.round((now - new Date(rental.rental_time)) / 60000);

  updateRow(TAB.rentals, "rental_id", rental.rental_id, {
    return_time: now, duration_min: durationMin, status: "returned"
  });
  updateRow(TAB.umbrellas, "umbrella_id", rental.umbrella_id, {
    status: "available", last_check_time: now
  });
  bumpAvailableSlots(rental.locker_id, +1);
  pushCommand("RETURN:" + rental.slot_no);

  return { ok: true, rental_id: rental.rental_id,
           slot_no: Number(rental.slot_no), duration_min: durationMin };
}

// 현황: 웹앱이 화면에 뿌릴 우산 목록
function status() {
  const um = sheetTable(TAB.umbrellas);
  return { ok: true, umbrellas: um.rows.map(r => ({
    umbrella_id: r.umbrella_id, locker_id: r.locker_id,
    slot_no: r.slot_no, status: r.status, last_user_id: r.last_user_id
  })) };
}

// ESP32 보고: 슬롯별 우산 유무(p1~p4) → last_check_time 갱신
function report(e) {
  const um = sheetTable(TAB.umbrellas);
  const now = new Date();
  let updated = 0;
  for (let slot = 1; slot <= 4; slot++) {
    const p = e.parameter["p" + slot];
    if (p === undefined) continue;
    const row = um.rows.find(r => parseInt(r.slot_no) === slot);
    if (row) { updateRow(TAB.umbrellas, "umbrella_id", row.umbrella_id,
                         { last_check_time: now }); updated++; }
  }
  return { ok: true, updated: updated };
}

// ─────────────────── 명령 큐 (ESP32 ↔ 시트) ───────────────────
function pushCommand(cmd) {
  ensureCommandsSheet().getRange("A1").setValue(cmd);
}
function popCommand() {          // 읽으면서 비우기 — 명령이 반복 실행되지 않게
  const cell = ensureCommandsSheet().getRange("A1");
  const cmd = String(cell.getValue() || "");
  if (cmd) cell.clearContent();
  return cmd;
}
function ensureCommandsSheet() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  return ss.getSheetByName(TAB.commands) || ss.insertSheet(TAB.commands);
}

// ─────────────────── 시트 헬퍼 (제목 행 기준) ───────────────────
// 열 순서가 바뀌어도 동작하도록 항상 "제목 행 이름"으로 읽고 씁니다.

function sheetTable(name) {
  const sh = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(name);
  if (!sh) throw "시트 탭이 없어요: " + name + " (TAB 설정 확인!)";
  const values = sh.getDataRange().getValues();
  const headers = values[0].map(h => String(h).trim());
  const rows = values.slice(1).map((v, i) => {
    const obj = { _row: i + 2 };               // 실제 시트 행 번호
    headers.forEach((h, c) => obj[h] = v[c]);
    return obj;
  });
  return { sheet: sh, headers: headers, rows: rows };
}

function findRow(name, key, value) {
  return sheetTable(name).rows.find(r => String(r[key]) === String(value)) || null;
}

function findActiveRental(userId) {
  return sheetTable(TAB.rentals).rows.find(r =>
    String(r.user_id) === String(userId) && r.status === "active") || null;
}

function updateRow(name, key, value, changes) {
  const t = sheetTable(name);
  const row = t.rows.find(r => String(r[key]) === String(value));
  if (!row) return false;
  for (const col in changes) {
    const c = t.headers.indexOf(col);
    if (c >= 0) t.sheet.getRange(row._row, c + 1).setValue(changes[col]);
  }
  return true;
}

function appendByHeader(name, obj) {
  const t = sheetTable(name);
  t.sheet.appendRow(t.headers.map(h => (h in obj) ? obj[h] : ""));
}

function bumpAvailableSlots(lockerId, delta) {
  const locker = findRow(TAB.lockers, "locker_id", lockerId);
  if (!locker) return;
  const next = Math.max(0, Number(locker.available_slots || 0) + delta);
  updateRow(TAB.lockers, "locker_id", lockerId,
            { available_slots: next, last_update: new Date() });
}

function nextId(name, key, prefix) {          // R001 → R002 …
  const rows = sheetTable(name).rows;
  let max = 0;
  rows.forEach(r => {
    const n = parseInt(String(r[key] || "").replace(prefix, ""));
    if (!isNaN(n) && n > max) max = n;
  });
  return prefix + String(max + 1).padStart(3, "0");
}

// ─────────────────── 응답 헬퍼 ───────────────────
function json(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}
function text(s) {
  return ContentService.createTextOutput(s);
}
