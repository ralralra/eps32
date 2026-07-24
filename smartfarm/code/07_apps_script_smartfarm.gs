/**
 * 스마트팜 — Apps Script 웹앱 (전체 코드)
 * 시트 구성: '시트1' 탭(기록) 1행 = 시각·토양습도·온도·습도
 *           '설정' 탭 A1 = 명령(AUTO/FAN_ON/FAN_OFF/LED_ON/LED_OFF)
 *                     B1 = 급수 알림 기준 토양습도(%) — 예: 30
 *
 * 주소 사용법 (URL = 배포한 /exec 주소):
 *   URL?soil=45&temp=25.3&humi=60      → 기록 저장 (ESP32가 10초마다)
 *   URL?mode=cmd                       → 설정!A1 명령 읽기 (ESP32)
 *   URL?mode=set&cmd=FAN_ON            → 설정!A1에 명령 쓰기 (앱 버튼)
 *   URL?mode=latest                    → 최신 측정값 JSON (앱 화면)
 *   URL?mode=limit&value=30            → 설정!B1 기준값 변경 (앱 슬라이더)
 *   URL?mode=getlimit                  → 설정!B1 기준값 읽기 (ESP32)
 */

function doGet(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const log = ss.getSheetByName("시트1");
  const cfg = ss.getSheetByName("설정");
  const p = e.parameter;

  // ── 명령 읽기 (ESP32 폴링) ─────────────────────
  if (p.mode == "cmd") {
    return ContentService.createTextOutput(String(cfg.getRange("A1").getValue()));
  }

  // ── 명령 쓰기 (앱 버튼) ────────────────────────
  if (p.mode == "set") {
    cfg.getRange("A1").setValue(p.cmd);
    return ContentService.createTextOutput("OK: " + p.cmd);
  }

  // ── 급수 알림 기준값 변경/읽기 ──────────────────
  if (p.mode == "limit") {
    cfg.getRange("B1").setValue(Number(p.value));
    return ContentService.createTextOutput("OK: limit=" + p.value);
  }
  if (p.mode == "getlimit") {
    return ContentService.createTextOutput(String(cfg.getRange("B1").getValue()));
  }

  // ── 최신 측정값 JSON (앱 화면) ─────────────────
  if (p.mode == "latest") {
    const r = log.getLastRow();
    if (r < 2) return ContentService.createTextOutput("{}");
    const v = log.getRange(r, 1, 1, 4).getValues()[0];
    const out = {
      time:  Utilities.formatDate(new Date(v[0]), "Asia/Seoul", "MM-dd HH:mm:ss"),
      soil:  v[1], temp: v[2], humi: v[3],
      limit: Number(cfg.getRange("B1").getValue()),
      cmd:   String(cfg.getRange("A1").getValue())
    };
    return ContentService.createTextOutput(JSON.stringify(out))
      .setMimeType(ContentService.MimeType.JSON);
  }

  // ── 기본: 측정값 기록 (ESP32 업로드) ────────────
  log.appendRow([new Date(), p.soil, p.temp, p.humi]);
  return ContentService.createTextOutput("SAVED");
}
