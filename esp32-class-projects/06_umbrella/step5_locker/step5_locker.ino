/*
  5단계 — 스마트 보관함 (완성본: 서보 4개 + 리드센서 4개 + 구글 시트 DB)
  ----------------------------------------------------------------------
  구조:
    [폰 웹앱] 대여/반납 버튼 → [Apps Script] 시트(DB) 갱신 + 명령 큐에 "RENT:슬롯"
        ↓ (ESP32가 3초마다 폴링)
    [ESP32] 명령 수신 → 서보 열림 → 리드센서로 우산 빠짐/거치 확인 → 서보 잠금
        ↓
    [ESP32] 30초마다 슬롯별 우산 유무를 시트에 보고 (last_check_time)

  배선: docs/wiring_servo_reed.md (서보 16·17·25·26 / 리드 13·14·21·22)
  라이브러리: ESP32Servo (라이브러리 매니저에서 "ESP32Servo" 검색)

  ★ 바꿀 곳: WiFi 이름·비번, 웹앱 URL (끝이 /exec!)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "WiFi이름";
const char* WIFI_PASS = "비밀번호";
const char* URL = "https://script.google.com/macros/s/XXXX/exec";

#define LED 2                                    // 내장 LED — 동작 확인용 신호등

const int SLOT_COUNT = 4;
const int SERVO_PIN[SLOT_COUNT] = { 16, 17, 25, 26 };   // 슬롯 1~4 잠금
const int REED_PIN[SLOT_COUNT]  = { 13, 14, 21, 22 };   // 슬롯 1~4 우산 감지

const int ANGLE_LOCKED = 0;      // ★ 기구에 맞게 조정
const int ANGLE_OPEN   = 90;

const unsigned long POLL_MS     = 3000;    // 명령 폴링 주기
const unsigned long REPORT_MS   = 30000;   // 슬롯 상태 보고 주기
const unsigned long DOOR_WAIT_MS = 20000;  // 문 열고 기다리는 최대 시간

Servo servos[SLOT_COUNT];
unsigned long lastPoll = 0, lastReport = 0;

// 리드센서: 자석 가까이(우산 있음) → LOW
bool umbrellaPresent(int i) { return digitalRead(REED_PIN[i]) == LOW; }

String httpGET(String query) {
  if (WiFi.status() != WL_CONNECTED) return "";
  HTTPClient http;
  http.begin(String(URL) + query);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  int code = http.GET();
  String body = (code > 0) ? http.getString() : "";
  http.end();
  return body;
}

void blinkOK() {                 // 성공 알림: 짧게 2회 (알림 문법 통일)
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED, HIGH); delay(150);
    digitalWrite(LED, LOW);  delay(150);
  }
}

void blinkWarn() {               // 경고: 빠른 연속 점멸
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED, HIGH); delay(80);
    digitalWrite(LED, LOW);  delay(80);
  }
}

// 슬롯별 우산 유무를 시트에 보고 (p1~p4: 1=있음, 0=없음)
void reportSlots() {
  String q = "?action=report";
  for (int i = 0; i < SLOT_COUNT; i++)
    q += "&p" + String(i + 1) + "=" + String(umbrellaPresent(i) ? 1 : 0);
  httpGET(q);
}

// 문 열기 → 리드센서 변화 대기 → 잠그기
//   takeOut=true  : 대여 — 우산이 "빠질 때"까지 기다림
//   takeOut=false : 반납 — 우산이 "꽂힐 때"까지 기다림
void openSlot(int slot, bool takeOut) {
  int i = slot - 1;
  if (i < 0 || i >= SLOT_COUNT) return;

  Serial.printf("슬롯 %d 열림 (%s 대기)\n", slot, takeOut ? "우산 빠짐" : "우산 거치");
  servos[i].write(ANGLE_OPEN);

  bool done = false;
  unsigned long start = millis();
  while (millis() - start < DOOR_WAIT_MS) {
    bool present = umbrellaPresent(i);
    if (takeOut && !present) { done = true; break; }   // 대여: 우산 빠짐 확인
    if (!takeOut && present) { done = true; break; }   // 반납: 우산 거치 확인
    delay(100);
  }

  delay(1500);                  // 손 빼고 문 닫을 여유
  servos[i].write(ANGLE_LOCKED);
  Serial.printf("슬롯 %d 잠김 — %s\n", slot, done ? "정상 처리" : "시간 초과!");

  if (done) blinkOK(); else blinkWarn();
  reportSlots();                // 처리 직후 최신 상태 보고
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  for (int i = 0; i < SLOT_COUNT; i++) {
    pinMode(REED_PIN[i], INPUT_PULLUP);
    servos[i].attach(SERVO_PIN[i]);
    servos[i].write(ANGLE_LOCKED);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi 연결 완료!");
  Serial.println("=== 스마트 우산 보관함 시작 ===");

  for (int i = 0; i < SLOT_COUNT; i++)
    Serial.printf("  슬롯 %d: 우산 %s\n", i + 1, umbrellaPresent(i) ? "있음" : "없음");
  reportSlots();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {     // 끊기면 재접속
    Serial.println("WiFi 재접속...");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  // ── ① 명령 폴링: "RENT:2" / "RETURN:3" ──────────────
  if (millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    String cmd = httpGET("?action=cmd");   // 읽으면 서버가 큐를 비움
    cmd.trim();
    if (cmd.startsWith("RENT:"))
      openSlot(cmd.substring(5).toInt(), true);
    else if (cmd.startsWith("RETURN:"))
      openSlot(cmd.substring(7).toInt(), false);
    else if (cmd.length() > 0)
      Serial.println("알 수 없는 명령: " + cmd);
  }

  // ── ② 주기적 상태 보고 ─────────────────────────────
  if (millis() - lastReport >= REPORT_MS) {
    lastReport = millis();
    reportSlots();
  }
}
