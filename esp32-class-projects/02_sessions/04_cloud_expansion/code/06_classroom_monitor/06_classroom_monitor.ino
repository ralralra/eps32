/*
  교실환경 모니터링 — 우리 교실 확정판 (실물 실드 배치 기준)
  빛·소리·온습도를 10초마다 구글 시트에 올리고,
  시트 '설정' 탭 A1의 ON/OFF로 LED를 원격 제어하고,
  현재 값을 LCD에도 보여줍니다.

  ── 배선 (실드 스티커 숫자 = GPIO) ───────────────────────
  빛 셀      → Analog 구역, 스티커 34 포트          → LIGHT 34
  소리 셀    → Analog 구역, 스티커 35 포트          → SOUND 35
  온습도 셀  → PWM 구역, 옆라벨 11 / 스티커 23 포트  → DHTPIN 23
              ⚠ 옆에 인쇄된 '11'은 우노 라벨! 코드에는 스티커 숫자 23!
  LED 셀     → JOY 구역, 옆라벨 9 / 스티커 13 포트   → LED 13
              (LED 셀이 없으면 내장 LED 사용: 13 → 2 로 변경)
  LCD 셀     → I2C 구역 (SDA 21 · SCL 22)

  ── 준비 (이미 하셨다면 통과!) ───────────────────────────
  시트: '시트1' 탭 1행 = 시각·온도·습도·조도·소음 / '설정' 탭 A1 = ON
  Apps Script: 03_apps_script_full.gs 배포(모든 사용자, /exec)

  ★ 바꿀 곳 3줄: WIFI_SSID · WIFI_PASS · URL
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define LIGHT  34  // 빛   (Analog · 스티커 34)
#define SOUND  35  // 소리 (Analog · 스티커 35)
#define DHTPIN 23  // 온습도 (PWM 구역 · 스티커 23 — '11' 아님!)
#define LED    13  // LED 셀 (JOY 구역 · 스티커 13)

const char* WIFI_SSID = "WiFi이름";   // ★ 2.4GHz만!
const char* WIFI_PASS = "비밀번호";   // ★
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ /exec 확인!

DHT dht(DHTPIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);   // 안 나오면 0x3F

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi OK!");
  lcd.clear();
  lcd.print("Cloud Ready!");
}

void loop() {
  // ── ① 센서 읽기 ──────────────────────────────
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   l = analogRead(LIGHT);   // 0~4095
  int   s = analogRead(SOUND);   // 0~4095

  // ── ② LCD에 현재 값 표시 ─────────────────────
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");  lcd.print(t, 1);
  lcd.print(" H:"); lcd.print(h, 0); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("L:");  lcd.print(l);
  lcd.print(" S:"); lcd.print(s);

  // ── ③ 구글 시트에 업로드 ─────────────────────
  HTTPClient http;
  String u = String(URL)
    + "?temp="  + String(t)
    + "&humi="  + String(h)
    + "&light=" + String(l)
    + "&sound=" + String(s);
  http.begin(u);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // 필수!
  int code = http.GET();
  http.end();
  Serial.printf("upload %d | T%.1f H%.0f L%d S%d\n", code, t, h, l, s);

  // ── ④ 명령 폴링: 설정!A1의 ON/OFF → LED ──────
  HTTPClient http2;
  http2.begin(String(URL) + "?mode=cmd");
  http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http2.GET();
  String cmd = http2.getString();
  http2.end();
  cmd.trim();                       // 앞뒤 공백 제거
  Serial.println("cmd: " + cmd);
  if (cmd == "ON")  digitalWrite(LED, HIGH);
  if (cmd == "OFF") digitalWrite(LED, LOW);

  delay(10000);  // 10초마다 반복
}
