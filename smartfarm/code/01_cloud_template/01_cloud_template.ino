/*
  스마트팜 — 클라우드 템플릿 (업로드 + 명령 폴링 + 자동 제어)
  10초마다: 센서 측정 → 시트 업로드 → 명령 확인 → 자동/수동 제어

  ── 배선 (기본안 — 키트에 맞게 조정, 우노 라벨은 변환표 참고!) ──
  토양습도  → A2 자리 (GPIO35)
  조도      → A3 자리 (GPIO34)
  수위      → A4 자리 (GPIO36)
  DHT11     → GPIO27 ('6' 칸)
  릴레이(펌프) IN → GPIO26   ⚠ 펌프 전원은 외부 5V! 보드에서 끌어오지 않기
  릴레이(팬)   IN → GPIO25
  LED(조명)      → GPIO17

  ── 명령 (시트 설정!A1) ──
  AUTO / PUMP_ON / PUMP_OFF / FAN_ON / FAN_OFF / LED_ON / LED_OFF

  ★ 바꿀 곳 3줄: WIFI_SSID · WIFI_PASS · URL
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define SOIL   35  // 토양습도 (A2 자리)
#define LIGHT  34  // 조도 (A3 자리)
#define LEVEL  36  // 수위 (A4 자리)
#define DHTPIN 27  // 온습도
#define PUMP   26  // 릴레이(펌프)
#define FAN    25  // 릴레이(팬)
#define LED    17  // 조명

#define LEVEL_MIN 800   // 수위가 이보다 낮으면 '물 부족' — 펌프 금지!
#define PUMP_SEC  5     // 한 번 급수는 최대 5초 (물 넘침 방지)

const char* WIFI_SSID = "WiFi이름";   // ★ 2.4GHz만!
const char* WIFI_PASS = "비밀번호";   // ★
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ /exec 확인!

DHT dht(DHTPIN, DHT11);

String httpGET(String url) {          // 요청 보내고 응답 문자열 받기
  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // 필수!
  http.GET();
  String body = http.getString();
  http.end();
  body.trim();
  return body;
}

void pumpOnce() {                     // 안전 급수: 최대 PUMP_SEC초만
  if (analogRead(LEVEL) < LEVEL_MIN) {
    Serial.println("물 부족! 급수 취소");
    return;
  }
  digitalWrite(PUMP, HIGH);
  delay(PUMP_SEC * 1000);
  digitalWrite(PUMP, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PUMP, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(LED, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi OK!");
}

void loop() {
  // ── ① 측정 ───────────────────────────────────
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil  = analogRead(SOIL);    // 낮을수록 마름 (센서에 따라 반대일 수 있음 — 1단계에서 확인!)
  int light = analogRead(LIGHT);
  int level = analogRead(LEVEL);
  Serial.printf("soil %d | T %.1f | H %.0f | light %d | level %d\n", soil, t, h, light, level);

  // ── ② 시트 업로드 ────────────────────────────
  String up = String(URL)
    + "?soil="   + String(soil)
    + "&temp="   + String(t)
    + "&humi="   + String(h)
    + "&light="  + String(light)
    + "&level="  + String(level);
  Serial.println("upload: " + httpGET(up));

  // ── ③ 명령 확인 ──────────────────────────────
  String cmd = httpGET(String(URL) + "?mode=cmd");
  Serial.println("cmd: " + cmd);

  if (cmd == "PUMP_ON")  { pumpOnce(); }               // 수동 급수 (안전 5초)
  if (cmd == "PUMP_OFF") { digitalWrite(PUMP, LOW); }
  if (cmd == "FAN_ON")   { digitalWrite(FAN, HIGH); }
  if (cmd == "FAN_OFF")  { digitalWrite(FAN, LOW); }
  if (cmd == "LED_ON")   { digitalWrite(LED, HIGH); }
  if (cmd == "LED_OFF")  { digitalWrite(LED, LOW); }

  // ── ④ 자동 모드: 마르면 급수, 더우면 환기 ─────
  if (cmd == "AUTO") {
    if (soil < 1500) {               // 기준값 — 1단계에서 우리 화분에 맞게!
      Serial.println("흙이 말랐어요 → 자동 급수");
      pumpOnce();
    }
    digitalWrite(FAN, (t >= 28) ? HIGH : LOW);   // 28℃ 이상이면 환기
  }

  delay(10000);  // 10초마다 반복
}
