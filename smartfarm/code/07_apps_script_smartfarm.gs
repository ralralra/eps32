/**
 * 스마트팜 — Apps Script 웹앱 (전체 코드)
 * 이 코드는 구글 시트에 붙여 "웹 창구(/exec 주소)"를 만들어요.
 * ESP32와 앱은 이 주소로 요청을 보내고, 아래 doGet이 요청을 받아 처리합니다.
 *
 * 시트 구성: '시트1' 탭(기록) 1행 = 시각·토양습도·온도·습도
 *           '설정' 탭 A1 = 명령(AUTO/FAN_ON/FAN_OFF/LED_ON/LED_OFF)
 *                     B1 = 급수 알림 기준 토양습도(%) — 예: 30
 *
 * 주소 사용법 (URL = 배포한 /exec 주소):
 *   URL?soil=45&temp=25.3&humi=60      → 기록 저장 + "명령,기준값" 응답 (ESP32가 30초마다)
 *   URL?mode=cmd                       → "명령,기준값" 읽기 — 예: AUTO,30 (ESP32가 3초마다)
 *   URL?mode=set&cmd=FAN_ON            → 설정!A1에 명령 쓰기 (앱 버튼)
 *   URL?mode=latest                    → 최신 측정값 JSON (앱 화면)
 *   URL?mode=limit&value=30            → 설정!B1 기준값 변경 (앱 슬라이더)
 *   URL?mode=getlimit                  → 설정!B1 기준값 읽기
 */

// 누군가 웹 주소로 GET 요청을 보낼 때마다 자동으로 실행되는 함수
function doGet(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();   // 이 스크립트가 붙어있는 스프레드시트
  const log = ss.getSheetByName("시트1");             // 기록 탭
  const cfg = ss.getSheetByName("설정");              // 설정 탭 (A1=명령, B1=기준값)
  const p = e.parameter;                              // 주소 뒤 ?이름=값 들이 여기 담겨요 (예: p.soil)

  // ── 명령 읽기 (ESP32 폴링) — "명령,기준값" 형태로 한 번에! ──
  if (p.mode == "cmd") {
    // A1의 명령과 B1의 기준값을 쉼표로 이어 붙여 응답 (예: "AUTO,30")
    const out = String(cfg.getRange("A1").getValue()) + "," + String(cfg.getRange("B1").getValue());
    return ContentService.createTextOutput(out);
  }

  // ── 명령 쓰기 (앱 버튼) ────────────────────────
  if (p.mode == "set") {
    cfg.getRange("A1").setValue(p.cmd);               // 설정!A1에 명령 저장 (예: FAN_ON)
    return ContentService.createTextOutput("OK: " + p.cmd);
  }

  // ── 급수 알림 기준값 변경/읽기 ──────────────────
  if (p.mode == "limit") {
    cfg.getRange("B1").setValue(Number(p.value));     // 설정!B1에 숫자로 저장
    return ContentService.createTextOutput("OK: limit=" + p.value);
  }
  if (p.mode == "getlimit") {
    return ContentService.createTextOutput(String(cfg.getRange("B1").getValue()));
  }

  // ── 최신 측정값 JSON (앱 화면 표시용) ─────────────
  if (p.mode == "latest") {
    const r = log.getLastRow();                       // 기록의 마지막 줄 번호
    if (r < 2) return ContentService.createTextOutput("{}");   // 기록이 아직 없으면 빈 응답
    const v = log.getRange(r, 1, 1, 4).getValues()[0];         // 마지막 줄의 4칸(시각~습도) 읽기
    const out = {                                     // 앱이 읽기 좋은 JSON 모양으로 포장
      time:  Utilities.formatDate(new Date(v[0]), "Asia/Seoul", "MM-dd HH:mm:ss"),
      soil:  v[1], temp: v[2], humi: v[3],
      limit: Number(cfg.getRange("B1").getValue()),   // 현재 기준값도 함께
      cmd:   String(cfg.getRange("A1").getValue())    // 현재 명령도 함께
    };
    return ContentService.createTextOutput(JSON.stringify(out))
      .setMimeType(ContentService.MimeType.JSON);
  }

  // ── 기본: 측정값 기록 (ESP32 업로드) ────────────
  // 응답에 "명령,기준값"을 실어 줘요 — 보드가 통신 1번으로 기록+명령확인을 한 번에!
  log.appendRow([new Date(), p.soil, p.temp, p.humi]);        // 맨 아래에 새 줄 추가
  const reply = String(cfg.getRange("A1").getValue()) + "," + String(cfg.getRange("B1").getValue());
  return ContentService.createTextOutput(reply);              // 예: "AUTO,30"
}
