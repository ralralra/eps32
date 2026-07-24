/**
 * 스마트팜 — Apps Script 웹앱 (전체 코드)
 * 시트 구성: '시트1' 탭(기록) 1행 = 시각·토양습도·온도·습도·조도·수위
 *           '설정' 탭 A1 = 명령(AUTO/PUMP_ON/…), B1 = 자동 급수 기준값
 *
 * 주소 사용법 (URL = 배포한 /exec 주소):
 *   URL?soil=1800&temp=25.3&humi=60&light=2000&level=3000  → 기록 저장
 *   URL?mode=cmd                → 설정!A1 명령 읽기 (ESP32가 10초마다)
 *   URL?mode=set&cmd=PUMP_ON    → 설정!A1에 명령 쓰기 (앱 버튼)
 *   URL?mode=latest             → 최신 측정값 JSON (앱 화면 표시용)
 *   URL?mode=limit&value=1600   → 설정!B1 기준값 변경 (앱 슬라이더)
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

  // ── 자동 급수 기준값 변경 ──────────────────────
  if (p.mode == "limit") {
    cfg.getRange("B1").setValue(Number(p.value));
    return ContentService.createTextOutput("OK: limit=" + p.value);
  }

  // ── 최신 측정값 JSON (앱 화면) ─────────────────
  if (p.mode == "latest") {
    const r = log.getLastRow();
    if (r < 2) return ContentService.createTextOutput("{}");
    const v = log.getRange(r, 1, 1, 6).getValues()[0];
    const out = {
      time:  Utilities.formatDate(new Date(v[0]), "Asia/Seoul", "MM-dd HH:mm:ss"),
      soil:  v[1], temp: v[2], humi: v[3], light: v[4], level: v[5],
      limit: Number(cfg.getRange("B1").getValue()),
      cmd:   String(cfg.getRange("A1").getValue())
    };
    return ContentService.createTextOutput(JSON.stringify(out))
      .setMimeType(ContentService.MimeType.JSON);
  }

  // ── 기본: 측정값 기록 (ESP32 업로드) ────────────
  log.appendRow([new Date(), p.soil, p.temp, p.humi, p.light, p.level]);
  return ContentService.createTextOutput("SAVED");
}
