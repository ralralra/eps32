/*
  4단계 — 클라우드 스마트팜 (완성판)
  10초마다: 측정 → 구글 시트 업로드 → 명령 확인 → 자동/수동 제어
  앱(AI Studio)에서 값 확인 + 팬·LED 제어 + 자동모드 기준값 변경!

  ── 배선 (05_auto_smartfarm과 동일) ──
  토양수분 → A2 자리(GPIO35) / DHT11 → D2 자리(GPIO26)
  팬 → 17·16·27·14 (모터드라이버) / 네오픽셀 → D9 자리(GPIO13)

  ── 명령 (시트 설정!A1 ← 앱이 씀) ──
  AUTO / FAN_ON / FAN_OFF / LED_ON / LED_OFF

  ★ 바꿀 곳 3줄: WIFI_SSID · WIFI_PASS · URL
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

#define SOIL   35
#define DHTPIN 26
#define AA 16
#define AB 17
#define BA 27
#define BB 14
#define LEDPIN 13
#define NUMLED 4

#define SOIL_MAX  4095  // 01_soil에서 찾은 최댓값으로!
#define TEMP_HIGH 26
#define TEMP_LOW  21
#define HUMI_HIGH 80

const char* WIFI_SSID = "WiFi이름";   // ★ 2.4GHz만!
const char* WIFI_PASS = "비밀번호";   // ★
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ /exec 확인!

DHT dht(DHTPIN, DHT11);
Adafruit_NeoPixel led(NUMLED, LEDPIN, NEO_GRB + NEO_KHZ800);

void fan(bool on) {
  digitalWrite(AA, on ? HIGH : LOW); digitalWrite(AB, LOW);
  digitalWrite(BA, on ? HIGH : LOW); digitalWrite(BB, LOW);
}

void warmLight(bool on) {
  for (int i = 0; i < NUMLED; i++)
    led.setPixelColor(i, on ? led.Color(255, 150, 60) : 0);
  led.show();
}

String httpGET(String url) {
  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // 필수!
  http.GET();
  String body = http.getString();
  http.end();
  body.trim();
  return body;
}

void setup() {
  Serial.begin(115200);
  pinMode(AA, OUTPUT); pinMode(AB, OUTPUT);
  pinMode(BA, OUTPUT); pinMode(BB, OUTPUT);
  dht.begin();
  led.begin();
  led.setBrightness(150);

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
  int soilPct = map(analogRead(SOIL), 0, SOIL_MAX, 0, 100);
  Serial.printf("T %.1f  H %.0f  Soil %d%%\n", t, h, soilPct);

  // ── ② 시트 업로드 ────────────────────────────
  String up = String(URL)
    + "?soil=" + String(soilPct)
    + "&temp=" + String(t)
    + "&humi=" + String(h);
  Serial.println("upload: " + httpGET(up));

  // ── ③ 명령 확인 (앱 → 시트 설정!A1 → 보드) ────
  String cmd = httpGET(String(URL) + "?mode=cmd");
  int dryLimit = httpGET(String(URL) + "?mode=getlimit").toInt();  // 급수 알림 기준(%)
  Serial.println("cmd: " + cmd + " / limit: " + String(dryLimit));

  if (cmd == "FAN_ON")  fan(true);
  if (cmd == "FAN_OFF") fan(false);
  if (cmd == "LED_ON")  warmLight(true);
  if (cmd == "LED_OFF") warmLight(false);

  // ── ④ 자동 모드 ──────────────────────────────
  if (cmd == "AUTO") {
    fan((t > TEMP_HIGH) || (h > HUMI_HIGH));
    warmLight(t < TEMP_LOW);
  }
  if (soilPct < dryLimit) Serial.println("💧 물 주세요! (앱에도 경고가 떠요)");

  delay(10000);  // 10초마다 반복
}
