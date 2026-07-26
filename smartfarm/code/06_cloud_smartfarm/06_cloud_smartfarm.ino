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

// 통신 및 센서 제어를 위해 필요한 라이브러리들을 불러온다
#include <WiFi.h>               // 와이파이 연결용
#include <HTTPClient.h>         // 웹(구글 시트) 통신용
#include <DHT.h>                // 온습도 센서 제어용
#include <Adafruit_NeoPixel.h>  // 네오픽셀 LED 제어용
#include <LiquidCrystal_I2C.h>  // I2C 방식 LCD 제어용

// 보드에 연결된 핀 번호들
#define SOIL   35   // 토양수분 센서 (확장쉴드 A2 줄)
#define DHTPIN 26   // 온습도 센서 (확장쉴드 D2 줄)
#define AA 16       // 모터드라이버 팬A 방향1 (D5 줄)
#define AB 17       // 모터드라이버 팬A 방향2 (D4 줄)
#define BA 27       // 모터드라이버 팬B 방향1 (D6 줄)
#define BB 14       // 모터드라이버 팬B 방향2 (D7 줄)
#define LEDPIN 13   // 네오픽셀 (D9 줄)
#define NUMLED 12   // 네오픽셀 LED 알갱이 개수

// 자동 제어 기준값
#define SOIL_DRY_RAW 0     // ★ 01_soil로 잰 '공기 중(마름)' 아날로그 값
#define SOIL_WET_RAW 4095  // ★ 01_soil로 잰 '물속(젖음)' 아날로그 값
#define TEMP_HIGH 26    // 이보다 더우면 팬
#define TEMP_LOW  21    // 이보다 추우면 보온등
#define HUMI_HIGH 80    // 이보다 습하면 환기(팬)
#define SOIL_WET  70    // 이보다 젖었으면 따뜻한 빛으로 말리기

// 와이파이·구글 시트 연결 정보 — ★ 내 것으로 바꾸세요!
const char* WIFI_SSID = "WiFi이름";   // ★ 와이파이 이름 (2.4GHz만 가능!)
const char* WIFI_PASS = "비밀번호";   // ★ 와이파이 비밀번호
const char* URL = "https://script.google.com/macros/s/XXXX/exec";  // ★ Apps Script 배포 주소 (/exec로 끝나는지 확인!)

// 센서·모듈 객체 생성
DHT dht(DHTPIN, DHT11);                                       // 온습도 센서 (핀, 종류)
Adafruit_NeoPixel led(NUMLED, LEDPIN, NEO_GRB + NEO_KHZ800);  // 네오픽셀
LiquidCrystal_I2C lcd(0x27, 16, 2);                           // LCD (주소 0x27, 16칸×2줄 — 안 나오면 0x3F)

// 팬 두 개를 켜고(true) 끄는(false) 함수
void fan(bool on) {
  // "on ? HIGH : LOW" = 조건 연산자: on이 참이면 HIGH, 거짓이면 LOW
  digitalWrite(AA, on ? HIGH : LOW); digitalWrite(AB, LOW);   // 팬A
  digitalWrite(BA, on ? HIGH : LOW); digitalWrite(BB, LOW);   // 팬B
}

// 네오픽셀을 생장등(보라색)으로 켜고(true) 끄는(false) 함수
void warmLight(bool on) {
  for (int i = 0; i < NUMLED; i++)                          // LED 12개를 하나씩
    led.setPixelColor(i, on ? led.Color(255, 0, 255) : 0);  // 켜면 보라색(생장등 색!), 끄면 0
  led.show();                                               // 실제 LED에 반영
}

// 주소(url)로 GET 요청을 보내고, 서버의 응답 문자열을 받아오는 함수
String httpGET(String url) {
  HTTPClient http;
  http.begin(url);                                          // 통신 준비
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);    // 구글의 주소 자동이동(리다이렉트)을 따라가기 — 필수!
  http.GET();                                               // 요청 발사!
  String body = http.getString();                           // 서버가 보낸 응답 본문 읽기
  http.end();                                               // 통신 마무리
  body.trim();                                              // 응답 앞뒤의 공백·줄바꿈 제거
  return body;                                              // 결과 돌려주기
}

// 처음 한 번만 실행되는 초기화 구간
void setup() {
  Serial.begin(115200);                      // 시리얼 통신 시작
  pinMode(AA, OUTPUT); pinMode(AB, OUTPUT);  // 모터드라이버 핀 4개를 출력용으로
  pinMode(BA, OUTPUT); pinMode(BB, OUTPUT);
  dht.begin();                               // 온습도 센서 시작
  led.begin();                               // 네오픽셀 시작
  led.setBrightness(150);                    // 밝기 150 (최대 255)
  lcd.init();                                // LCD 초기화
  lcd.backlight();                           // 백라이트 켜기
  lcd.print("WiFi...");                      // 연결 중 표시

  WiFi.begin(WIFI_SSID, WIFI_PASS);          // 와이파이 연결 시도
  while (WiFi.status() != WL_CONNECTED) {    // 연결될 때까지 기다리기
    delay(500);
    Serial.print(".");                       // 0.5초마다 점 찍기
  }
  Serial.println(" WiFi OK!");               // 연결 성공!
  lcd.clear();
  lcd.print("Smart Farm OK!");               // LCD에도 성공 표시
}

// ── 최근 상태를 담아두는 변수들 (loop가 돌 때마다 새로 만들지 않고 계속 기억) ──
float t = 0, h = 0;        // 온도·습도
int soilPct = 0;           // 토양습도(%)
String cmd = "AUTO";       // 현재 명령 (시작은 자동 모드)
int dryLimit = 30;         // 급수 알림 기준(%) — 앱 슬라이더로 바뀌어요
unsigned long lastRead = 0, lastPoll = 0, lastUpload = 0;   // 세 가지 일의 "마지막 실행 시각"

// 서버 응답("AUTO,30" 모양)에서 명령·기준값을 꺼내 적용하는 함수
// — 명령 폴링과 업로드가 같은 모양으로 응답하므로 둘 다 이 함수 하나로!
void handleReply(String r) {
  int comma = r.indexOf(',');        // 쉼표(,)의 위치 찾기
  if (comma <= 0) return;            // 쉼표가 없으면(통신 실패 등) 이전 값 유지하고 종료
  cmd = r.substring(0, comma);                  // 쉼표 앞부분 = 명령 (예: "AUTO")
  dryLimit = r.substring(comma + 1).toInt();    // 쉼표 뒷부분 = 기준값 (예: 30, 문자열→정수 변환)

  // 수동 명령이면 바로 실행
  if (cmd == "FAN_ON")  fan(true);       // 팬 강제 켜기
  if (cmd == "FAN_OFF") fan(false);      // 팬 강제 끄기
  if (cmd == "LED_ON")  warmLight(true); // 생장등 강제 켜기
  if (cmd == "LED_OFF") warmLight(false);// 생장등 강제 끄기
}

// 무한 반복 구간 — millis()는 "전원 켜진 뒤 지난 시간(ms)"을 알려주는 시계예요.
// delay와 달리 기다리는 동안 다른 일을 할 수 있어요!
void loop() {
  // ── ① 2초마다: 측정 + LCD + 자동 제어 ─────────
  if (millis() - lastRead >= 2000) {     // 마지막 측정 후 2초가 지났으면
    lastRead = millis();                 // 지금 시각을 기록해 두고
    float nt = dht.readTemperature();    // 새 온도 읽기
    float nh = dht.readHumidity();       // 새 습도 읽기
    if (!isnan(nt)) t = nt;              // 측정 성공했을 때만 저장 (실패=nan이면 이전 값 유지)
    if (!isnan(nh)) h = nh;
    // 2점 보정: 마른 값→0%, 젖은 값→100% (방향이 반대인 센서도 map이 알아서 처리)
    soilPct = constrain(map(analogRead(SOIL), SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100), 0, 100);
    Serial.printf("T %.1f  H %.0f  Soil %d%%  cmd:%s\n", t, h, soilPct, cmd.c_str());

    // 자동 모드일 때만: 더우면·습하면 팬 / 추우면·과습이면 생장등 (05와 같은 규칙!)
    if (cmd == "AUTO") {
      fan((t > TEMP_HIGH) || (h > HUMI_HIGH));               // || 는 "또는"
      warmLight((t < TEMP_LOW) || (soilPct > SOIL_WET));
    }

    // LCD 상황판 — 1줄: 온습도 / 2줄: 토양습도 (+ 급수 알림)
    lcd.clear();                          // 이전 글자 지우기
    lcd.setCursor(0, 0);                  // 첫째 줄 처음으로
    lcd.print("T:");   lcd.print(t, 1);   // 온도 (소수점 1자리)
    lcd.print(" H:");  lcd.print(h, 0); lcd.print("%");  // 습도 (정수)
    lcd.setCursor(0, 1);                  // 둘째 줄 처음으로
    lcd.print("Soil:"); lcd.print(soilPct); lcd.print("%");  // 토양습도
    if (soilPct < dryLimit) {             // 기준보다 마르면
      lcd.setCursor(9, 1);                // 둘째 줄 오른쪽에
      lcd.print("WATER!");                // 💧 물 주세요! (앱에도 경고가 떠요)
    }
  }

  // ── ② 3초마다: 앱 명령 확인 (빠른 반응!) ──────
  if (millis() - lastPoll >= 3000) {
    lastPoll = millis();
    handleReply(httpGET(String(URL) + "?mode=cmd"));   // 응답: "AUTO,30" (명령,기준값)
  }

  // ── ③ 30초마다: 구글 시트에 기록 ──────────────
  //    업로드 응답에도 "명령,기준값"이 실려 와요 — 통신 1번으로 두 가지 일!
  if (millis() - lastUpload >= 30000) {
    lastUpload = millis();
    lastPoll = millis();                 // 방금 명령도 받았으니 다음 폴링은 3초 뒤로 미루기
    // 주소 뒤에 ?이름=값&이름=값 형태로 측정값을 붙여서 전송
    String up = String(URL)
      + "?soil=" + String(soilPct)
      + "&temp=" + String(t)
      + "&humi=" + String(h);
    String r = httpGET(up);              // 전송하고 응답 받기
    Serial.println("upload: " + r);      // 응답 확인 (예: "AUTO,30")
    handleReply(r);                      // 응답 속 명령·기준값도 적용
  }
}
