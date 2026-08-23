/*
  5단계 — 스마트 보관함 (서보 4개 + 리드스위치 4개, 센서쉴드 버전)
  ----------------------------------------------------------------------
  슬롯마다 서보(잠금)와 리드스위치(우산 감지)가 짝을 이룹니다.
    리드스위치 감지(자석 가까이) = 우산 있음 / 감지 안 됨 = 우산 없음

  ● 대여: 명령 → 열림(100도) → 5초 → 닫힘(20도) → 리드 확인
       우산 없음 → ✓ 대여완료
       우산 그대로 → ✗ 대여실패 (시트의 대여 기록 자동 취소)
  ● 반납: 명령 → 열림 → 5초 → 닫힘 → 리드 확인
       우산 감지 → ✓ 반납완료
       감지 안 됨 → 다시 열렸다가 닫힘 (최대 3회 반복 후 실패 처리)

  명령은 두 가지 방법으로 들어옵니다:
    ① 앱: Apps Script 명령 큐를 3초마다 폴링 ("RENT:2" / "RETURN:3")
    ② 시리얼 모니터(115200): r1~r4 = 대여, b1~b4 = 반납, s = 상태 보기
       (서보 점검: o1~o4 = 그냥 열기, c1~c4 = 그냥 닫기)
       (WiFi 없이 시리얼만으로도 전체 테스트 가능 — 15초 후 오프라인 모드)

  배선: docs/wiring_servo_reed.md — 센서쉴드에 그대로 꽂으면 됩니다
    서보 신호: GPIO16·17·25·26 (실드 숫자 5·4·3·2)
    리드:      GPIO13·14·27·4  (실드 숫자 9·7·6·A1)
  ⚠ 서보 전원은 실드의 외부 전원 단자에 5V — 보드 전원으로 4개 돌리면 리셋돼요!

  라이브러리: ESP32Servo (기본 Servo.h는 ESP32에서 안 됩니다)
  ★ 바꿀 곳: WiFi 이름·비번, 웹앱 URL (끝이 /exec!)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "WiFi이름";
const char* WIFI_PASS = "비밀번호";
const char* URL = "https://script.google.com/macros/s/XXXX/exec";

// 이 보드가 담당하는 보관함 번호 — 시트 lockers 탭의 locker_id와 같아야 해요.
// 슬롯 번호(1~4)는 보관함마다 겹치기 때문에 이 값으로 구분합니다.
const char* LOCKER_ID = "L001";

#define LED 2                                    // 내장 LED — 동작 확인용

const int SLOT_COUNT = 4;
const int SERVO_PIN[SLOT_COUNT] = { 16, 17, 25, 26 };   // 슬롯 1~4 (실드 5·4·3·2)
const int REED_PIN[SLOT_COUNT]  = { 13, 14, 27, 4 };    // 슬롯 1~4 (실드 9·7·6·A1)

const int ANGLE_CLOSED = 20;                // 닫힘(잠김) 각도
const int ANGLE_OPEN   = 100;               // 열림 각도
const unsigned long OPEN_TIME_MS  = 5000;   // 열려 있는 시간 (5초)
const unsigned long SETTLE_MS     = 600;    // 서보가 닫힐 때까지 잠깐 대기
const int RETURN_RETRY_MAX = 3;             // 반납 시 다시 열어주는 최대 횟수

// 평소(대기 중)에 서보의 힘을 빼둘지 — 켜두면 전류를 훨씬 덜 먹고 떨림·발열도 줍니다.
// 서보가 계속 버티는 힘이 필요하면(문을 손으로 못 열게) false로 바꾸세요.
const bool RELEASE_WHEN_IDLE = true;

const unsigned long POLL_MS   = 3000;       // 앱 명령 폴링 주기
const unsigned long REPORT_MS = 30000;      // 슬롯 상태 보고 주기

Servo servos[SLOT_COUNT];
bool online = false;                        // WiFi 연결 여부 (없어도 시리얼은 동작)
unsigned long lastPoll = 0, lastReport = 0;

// 리드스위치: 자석 가까이(우산 있음) → LOW
bool umbrellaPresent(int i) { return digitalRead(REED_PIN[i]) == LOW; }

// 서보는 붙어 있는 동안 계속 힘을 씁니다. 필요할 때만 붙이고 끝나면 떼어내요.
void servoMove(int i, int angle) {
  if (!servos[i].attached()) servos[i].attach(SERVO_PIN[i], 500, 2400);
  servos[i].write(angle);
}
void servoRelease(int i) {
  if (RELEASE_WHEN_IDLE && servos[i].attached()) servos[i].detach();
}

String httpGET(String query) {
  if (!online) return "";
  HTTPClient http;
  http.begin(String(URL) + query);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  int code = http.GET();
  String body = (code > 0) ? http.getString() : "";
  http.end();
  return body;
}

void blinkOK() {                 // 성공: 짧게 2회
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED, HIGH); delay(150);
    digitalWrite(LED, LOW);  delay(150);
  }
}
void blinkWarn() {               // 실패·경고: 빠른 연속 점멸
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED, HIGH); delay(80);
    digitalWrite(LED, LOW);  delay(80);
  }
}

void printStatus() {
  Serial.println("── 슬롯 상태 ──");
  for (int i = 0; i < SLOT_COUNT; i++)
    Serial.printf("  슬롯 %d: 우산 %s\n", i + 1, umbrellaPresent(i) ? "있음" : "없음");
}

// 슬롯별 우산 유무를 시트에 보고 (p1~p4: 1=있음, 0=없음)
void reportSlots() {
  String q = "?action=report&locker_id=" + String(LOCKER_ID);
  for (int i = 0; i < SLOT_COUNT; i++)
    q += "&p" + String(i + 1) + "=" + String(umbrellaPresent(i) ? 1 : 0);
  httpGET(q);
}

// 열림 → 5초 → 닫힘 (한 사이클) — 각 단계를 시리얼에 찍어 눈으로 확인할 수 있게
void openCloseCycle(int i) {
  Serial.printf("슬롯%d 열림\n", i + 1);
  servoMove(i, ANGLE_OPEN);
  delay(OPEN_TIME_MS);

  Serial.printf("%lu초후 닫힘\n", OPEN_TIME_MS / 1000);
  servoMove(i, ANGLE_CLOSED);
  delay(SETTLE_MS);              // 닫힌 뒤 리드 값이 안정될 때까지
  servoRelease(i);               // 다 움직였으면 힘 빼기 (전류·떨림 감소)
}

// 수동 열기/닫기 — 서보·전원 점검용 (대여/반납 처리 없이 모터만 움직임)
void doOpen(int slot) {
  int i = slot - 1;
  if (i < 0 || i >= SLOT_COUNT) { Serial.println("슬롯 번호는 1~4 입니다"); return; }
  Serial.printf("슬롯%d 열림 (닫으려면 c%d)\n", slot, slot);
  servoMove(i, ANGLE_OPEN);      // 열어둔 채 유지 (전압 재보기 좋게)
}
void doClose(int slot) {
  int i = slot - 1;
  if (i < 0 || i >= SLOT_COUNT) { Serial.println("슬롯 번호는 1~4 입니다"); return; }
  Serial.printf("슬롯%d 닫힘\n", slot);
  servoMove(i, ANGLE_CLOSED);
  delay(SETTLE_MS);
  servoRelease(i);
}

// ── 대여 ──────────────────────────────────────────────
void doRent(int slot) {
  int i = slot - 1;
  if (i < 0 || i >= SLOT_COUNT) { Serial.println("슬롯 번호는 1~4 입니다"); return; }

  if (!umbrellaPresent(i)) {     // 빈 슬롯은 빌려줄 우산이 없음
    Serial.printf("슬롯%d 대여불가 — 우산이 없어요\n", slot);
    blinkWarn();
    httpGET("?action=cancel&locker_id=" + String(LOCKER_ID) + "&slot=" + String(slot));   // 시트의 대여 기록 취소
    return;
  }

  openCloseCycle(i);

  if (!umbrellaPresent(i)) {
    Serial.println("대여완료");
    blinkOK();
  } else {
    Serial.println("대여실패 — 우산이 그대로 있어요");
    blinkWarn();
    httpGET("?action=cancel&locker_id=" + String(LOCKER_ID) + "&slot=" + String(slot));   // DB도 원래대로 되돌리기
  }
  reportSlots();
}

// ── 반납 ──────────────────────────────────────────────
void doReturn(int slot) {
  int i = slot - 1;
  if (i < 0 || i >= SLOT_COUNT) { Serial.println("슬롯 번호는 1~4 입니다"); return; }

  for (int attempt = 1; attempt <= RETURN_RETRY_MAX; attempt++) {
    openCloseCycle(i);

    if (umbrellaPresent(i)) {
      Serial.println("반납완료");
      blinkOK();
      reportSlots();
      return;
    }
    Serial.println("우산이 없어요 — 다시 열게요");
  }
  Serial.printf("반납실패 — %d번 열어도 우산이 없어요\n", RETURN_RETRY_MAX);
  blinkWarn();
  reportSlots();
}

// ── 명령 해석 (앱 폴링·시리얼 공용) ─────────────────────
//   RENT:2 / RETURN:3  또는  r2 / b3 / s
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  if (cmd == "S")                    { printStatus(); return; }
  if (cmd.startsWith("RENT:"))       { doRent(cmd.substring(5).toInt());    return; }
  if (cmd.startsWith("RETURN:"))     { doReturn(cmd.substring(7).toInt());  return; }
  if (cmd[0] == 'R' && cmd.length() == 2) { doRent(cmd.substring(1).toInt());   return; }
  if (cmd[0] == 'B' && cmd.length() == 2) { doReturn(cmd.substring(1).toInt()); return; }
  if (cmd[0] == 'O' && cmd.length() == 2) { doOpen(cmd.substring(1).toInt());    return; }
  if (cmd[0] == 'C' && cmd.length() == 2) { doClose(cmd.substring(1).toInt());   return; }

  Serial.println("모르는 명령: " + cmd +
                 "  (r1~r4=대여, b1~b4=반납, o1~o4=열기, c1~c4=닫기, s=상태)");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(LED, OUTPUT);

  // 시작할 때 네 칸을 잠그되, 한 개씩 차례로 — 동시에 움직이면 전류가 확 튀어
  // 약한 전원에서는 보드가 리셋됩니다.
  Serial.println("\n=== 스마트 우산 보관함 ===");
  Serial.println("네 칸을 차례로 잠급니다...");
  for (int i = 0; i < SLOT_COUNT; i++) {
    pinMode(REED_PIN[i], INPUT_PULLUP);
    servos[i].setPeriodHertz(50);
    Serial.printf("슬롯%d 잠금\n", i + 1);
    servoMove(i, ANGLE_CLOSED);
    delay(400);
    servoRelease(i);
  }

  Serial.println("시리얼 명령");
  Serial.println("  r1~r4 = 대여   b1~b4 = 반납   s = 슬롯 상태");
  Serial.println("  o1~o4 = 그냥 열기   c1~c4 = 그냥 닫기  (서보·전원 점검용)");

  // WiFi는 15초만 시도 — 안 되면 시리얼 전용(오프라인)으로 계속
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  online = (WiFi.status() == WL_CONNECTED);
  Serial.println(online ? " WiFi 연결 완료 — 앱 명령 대기!"
                        : " WiFi 없음 — 시리얼 전용 모드");

  printStatus();
  reportSlots();
}

void loop() {
  // ── ① 시리얼 명령 ──
  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }

  // ── ①-2 부팅 때 WiFi를 못 잡았어도 나중에 잡히면 자동으로 온라인 전환 ──
  if (!online && millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    if (WiFi.status() == WL_CONNECTED) {
      online = true;
      Serial.println("WiFi 연결됨 — 앱 명령을 받기 시작합니다");
      reportSlots();
    }
  }

  // ── ② 앱 명령 폴링 ──
  if (online && millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    if (WiFi.status() != WL_CONNECTED) {   // 끊기면 재접속 시도
      Serial.println("WiFi 재접속...");
      WiFi.reconnect();
    } else {
      String cmd = httpGET("?action=cmd"); // 읽으면 서버가 큐를 비움
      if (cmd.length() > 0) handleCommand(cmd);
    }
  }

  // ── ③ 주기적 상태 보고 ──
  if (online && millis() - lastReport >= REPORT_MS) {
    lastReport = millis();
    reportSlots();
  }
}
