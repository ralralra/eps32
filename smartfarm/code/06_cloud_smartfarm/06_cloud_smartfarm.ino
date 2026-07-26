/*
  4단계 — 클라우드 스마트팜 (완성판)
  일은 세 가지 리듬으로 나눠서 해요 (delay 대신 millis 시계 사용):
    2초마다  → 센서 측정 + LCD + 자동 제어
    3초마다  → 앱 명령 확인 (버튼 반응이 빨라져요!)
    30초마다 → 구글 시트에 기록 (기록은 천천히 쌓여도 충분)

  ※ 앱 버튼 → 실제 동작까지 3~6초 정도 걸리는 게 정상이에요.
    (앱→시트는 즉시, 보드가 시트를 3초마다 확인 + 통신 왕복 시간)

  ── 배선 (05_auto_smartfarm과 동일) ──
  토양수분 → A2 자리(GPIO35) / DHT11 → D2 자리(GPIO26)
  팬 → 17·16·27·14 (모터드라이버) / 네오픽셀 → D9 자리(GPIO13)
  LCD → SDA·SCL을 보드 핀(21·22)에 직접! (쉴드 A4·A5 줄은 안 돼요)

  ── 명령 (시트 설정!A1 ← 앱이 씀) ──
  AUTO / FAN_ON / FAN_OFF / LED_ON / LED_OFF

  ★ 바꿀 곳 3줄: WIFI_SSID · WIFI_PASS · URL
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>

#define SOIL   35
#define DHTPIN 26
#define AA 16
#define AB 17
#define BA 27
#define BB 14
#define LEDPIN 13
#define NUMLED 12

#define SOIL_MAX  4095  // 01_soil에서 찾은 최댓값으로!
#define TEMP_HIGH 26
#define TEMP_LOW  21
#define HUMI_HIGH 80
#define SOIL_WET  70    // 이보다 젖었으면 따뜻한 빛으로 말리기

const char* WIFI_SSID = "WiFi이름";   // ★ 2.4GHz만!
const char* WIFI_PASS = "비밀번호";   // ★
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ /exec 확인!

DHT dht(DHTPIN, DHT11);
Adafruit_NeoPixel led(NUMLED, LEDPIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);   // 키트 LCD 주소 = 0x27 (안 나오면 0x3F)

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
  lcd.print("Smart Farm OK!");
}

// 최근 상태를 담아두는 변수들
float t = 0, h = 0;
int soilPct = 0;
String cmd = "AUTO";
int dryLimit = 30;
unsigned long lastRead = 0, lastPoll = 0, lastUpload = 0;

void loop() {
  // ── ① 2초마다: 측정 + LCD + 자동 제어 ─────────
  if (millis() - lastRead >= 2000) {
    lastRead = millis();
    float nt = dht.readTemperature();
    float nh = dht.readHumidity();
    if (!isnan(nt)) t = nt;          // 측정 실패(nan)면 이전 값 유지
    if (!isnan(nh)) h = nh;
    soilPct = constrain(map(analogRead(SOIL), 0, SOIL_MAX, 0, 100), 0, 100);
    Serial.printf("T %.1f  H %.0f  Soil %d%%  cmd:%s\n", t, h, soilPct, cmd.c_str());

    // 자동 모드: 더우면·습하면 팬 / 추우면·과습이면 따뜻한 빛 (05와 같은 규칙!)
    if (cmd == "AUTO") {
      fan((t > TEMP_HIGH) || (h > HUMI_HIGH));
      warmLight((t < TEMP_LOW) || (soilPct > SOIL_WET));
    }

    // LCD 상황판 — 1줄: 온습도 / 2줄: 토양습도 (+ 급수 알림)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");   lcd.print(t, 1);
    lcd.print(" H:");  lcd.print(h, 0); lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Soil:"); lcd.print(soilPct); lcd.print("%");
    if (soilPct < dryLimit) {
      lcd.setCursor(9, 1);
      lcd.print("WATER!");             // 💧 물 주세요! (앱에도 경고가 떠요)
    }
  }

  // ── ② 3초마다: 앱 명령 확인 (빠른 반응!) ──────
  if (millis() - lastPoll >= 3000) {
    lastPoll = millis();
    String r = httpGET(String(URL) + "?mode=cmd");   // 응답 모양: "AUTO,30" (명령,기준값)
    int comma = r.indexOf(',');
    if (comma > 0) {
      cmd = r.substring(0, comma);
      dryLimit = r.substring(comma + 1).toInt();
    }
    if (cmd == "FAN_ON")  fan(true);
    if (cmd == "FAN_OFF") fan(false);
    if (cmd == "LED_ON")  warmLight(true);
    if (cmd == "LED_OFF") warmLight(false);
  }

  // ── ③ 30초마다: 구글 시트에 기록 ──────────────
  if (millis() - lastUpload >= 30000) {
    lastUpload = millis();
    String up = String(URL)
      + "?soil=" + String(soilPct)
      + "&temp=" + String(t)
      + "&humi=" + String(h);
    Serial.println("upload: " + httpGET(up));
  }
}
