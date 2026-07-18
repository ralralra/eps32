# 연결 요령 — action 하나를 끝까지 뚫기 (매점팀 예시)

> 다른 팀도 **이름만 바꾸면 같은 구조**예요. 한 구간씩 시리얼 모니터·시트를 보며 확인!

## 전체 흐름

```
[폰 웹앱] 판매 버튼 터치
    ▼  fetch(URL + "?action=sell&item=cola")
[Apps Script] sell() 실행 — 판매로그 기록 + 재고 차감 + 명령 셀에 "SOLD:cola"
    ▼  (ESP32가 10초마다 폴링)
[ESP32] "SOLD:" 발견 → LCD 표시 + LED 깜빡 → A1 비우기 요청
    ▼
[시트] 판매로그에 새 줄 — 엔드 투 엔드 완성!
```

## ① 웹앱 JS — 판매 버튼

```javascript
fetch(URL + "?action=sell&item=cola");
// 웹은 action만 던져요 — 판단은 서버(Apps Script)가!
```

## ② Apps Script — sell 함수

```javascript
function sell(e) {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const log = ss.getSheetByName("판매로그");
  const cmd = ss.getSheetByName("설정");

  const item = e.parameter.item;
  log.appendRow([new Date(), item, 1]);   // 판매로그 1줄

  // 상품 시트에서 해당 행 재고 -1 처리 (getValues로 찾아 setValue)

  cmd.getRange("A1").setValue("SOLD:" + item);  // 하드웨어 알림은 명령 셀로!
  return ContentService.createTextOutput("OK");
}
```

## ③ ESP32 — 폴링에서 SOLD: 발견 시

```cpp
// loop()의 명령 폴링 부분 (4회차 템플릿)에 추가:
if (cmd.startsWith("SOLD:")) {
  String item = cmd.substring(5);      // "SOLD:cola" → "cola"
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SOLD: " + item);
  for (int i = 0; i < 2; i++) {        // 성공 알림: 짧게 2회
    digitalWrite(LED, HIGH); delay(150);
    digitalWrite(LED, LOW);  delay(150);
  }
  // ★ 처리 후 A1 비우기 — 안 비우면 알림이 계속 반복!
  HTTPClient hc;
  hc.begin(String(URL) + "?mode=set&cmd=");
  hc.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  hc.GET();
  hc.end();
}
```

## 팀별 이름 교체표

| 팀 | action | 명령 셀 값 예시 | LCD 표시 |
|---|---|---|---|
| ① 매점 | `sell` | `SOLD:cola` | `SOLD: cola` |
| ② 키오스크 | `order` | `ORDER:17` | 주문번호 크게 |
| ③ 우산 | `rent` / `return` | `RENT:3` | `Rent OK #3` |
| ④ 화분 | (센서 감지가 출발점) | `WATER!` | `급수 필요!` → `Water Me!` |
| ⑤ 출석 | `checkin` | `IN:철수` | 현재 인원 |

> ⚠ LCD는 한글 표시 불가 — 문구는 영문·숫자로!
> ✓ 막히면 구간 분리 점검: ① `URL?action=…` 브라우저로 직접 열기 ② 시트 명령 셀 변하나 ③ ESP32 시리얼에 폴링 로그 찍히나
