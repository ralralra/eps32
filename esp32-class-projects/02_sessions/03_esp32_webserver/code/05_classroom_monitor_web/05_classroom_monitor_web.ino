/*
  교실환경 모니터링 — 웹서버 버전 (3회차 방식 · 인터넷 불필요!)
  같은 값을 2곳에서 확인해요:
    ① 시리얼 모니터(115200)  ② 폰 웹페이지(192.168.4.1)

  ── 배선 (실드 스티커 숫자 = GPIO) ───────────────────────
  빛 셀      → Analog 구역, 스티커 34 포트          → LIGHT 34
  소리 셀    → Analog 구역, 스티커 35 포트          → SOUND 35
  온습도 셀  → PWM 구역, 옆라벨 11 / 스티커 23 포트  → DHTPIN 23
              ⚠ 옆에 인쇄된 '11'은 우노 라벨! 코드에는 스티커 숫자 23!
  LED 셀     → JOY 구역, 옆라벨 9 / 스티커 13 포트   → LED 13

  ── 사용법 ───────────────────────────────────────────────
  1. WiFi 이름만 팀 것으로 바꾸고 업로드
  2. 시리얼 모니터(115200)에서 값 확인  ← 1차 확인
  3. 폰 WiFi를 Team1_IoT(비번 12345678)에 연결
     → 브라우저에서 192.168.4.1        ← 2차 확인 + LED 버튼!
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define LIGHT  34  // 빛   (Analog · 스티커 34)
#define SOUND  35  // 소리 (Analog · 스티커 35)
#define DHTPIN 23  // 온습도 (PWM 구역 · 스티커 23 — '11' 아님!)
#define LED    13  // LED 셀 (JOY 구역 · 스티커 13)

DHT dht(DHTPIN, DHT11);
WebServer server(80);

// 최근 측정값 (2초마다 갱신해서 여기 저장)
float gT = 0, gH = 0;
int   gL = 0, gS = 0;
unsigned long lastRead = 0;

const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>교실 환경 모니터</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #EDF3FE; }
    h1 { color: #2E6BE6; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px;
            max-width: 420px; margin: 0 auto; }
    .card { background: white; border-radius: 14px; padding: 18px; }
    .card b { font-size: 34px; color: #1B2536; }
    .card span { color: #56677E; font-size: 15px; }
    button {
      font-size: 22px; padding: 16px 32px; margin: 14px 8px 0;
      border-radius: 12px; border: none; background: #2E6BE6; color: white; }
    button.off { background: #8A99AC; }
  </style>
</head>
<body>
  <h1>교실 환경 모니터</h1>
  <div class="grid">
    <div class="card"><span>온도</span><br><b id="t">--</b> ℃</div>
    <div class="card"><span>습도</span><br><b id="h">--</b> %</div>
    <div class="card"><span>조도</span><br><b id="l">--</b></div>
    <div class="card"><span>소음</span><br><b id="s">--</b></div>
  </div>
  <button onclick="fetch('/led/on')">LED 켜기</button>
  <button class="off" onclick="fetch('/led/off')">LED 끄기</button>
  <script>
    async function update() {                  // 폴링: 2초마다 '지금 어때?'
      const res = await fetch("/sensor");
      const d = await res.json();
      document.getElementById("t").innerText = d.temp;
      document.getElementById("h").innerText = d.humi;
      document.getElementById("l").innerText = d.light;
      document.getElementById("s").innerText = d.sound;
    }
    setInterval(update, 2000);
    update();
  </script>
</body>
</html>
)rawliteral";

void readSensors() {   // 2초마다 호출 — 측정 + 시리얼 출력 한 번에
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) gT = t;           // nan(측정 실패)이면 이전 값 유지
  if (!isnan(h)) gH = h;
  gL = analogRead(LIGHT);
  gS = analogRead(SOUND);

  // ① 시리얼 모니터로 확인
  Serial.printf("T:%.1fC  H:%.0f%%  L:%d  S:%d\n", gT, gH, gL, gS);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  dht.begin();

  WiFi.softAP("Team1_IoT", "12345678");   // ★ 팀 이름으로! (비번 8자 이상)
  Serial.print("접속 주소: http://");
  Serial.println(WiFi.softAPIP());        // 192.168.4.1

  server.on("/", []() {                   // ② 웹페이지로 확인
    server.send(200, "text/html", PAGE);
  });
  server.on("/sensor", []() {             // 값을 JSON으로 응답
    String json = "{\"temp\":"   + String(gT, 1)
                + ",\"humi\":"   + String(gH, 0)
                + ",\"light\":"  + String(gL)
                + ",\"sound\":"  + String(gS) + "}";
    server.send(200, "application/json", json);
  });
  server.on("/led/on", []() {
    digitalWrite(LED, HIGH);
    server.send(200, "text/plain", "OK");
  });
  server.on("/led/off", []() {
    digitalWrite(LED, LOW);
    server.send(200, "text/plain", "OK");
  });
  server.begin();
}

void loop() {
  server.handleClient();                  // 손님(폰) 계속 응대

  // delay 대신 시계(millis)로 2초마다 측정 — 웹 응답이 느려지지 않아요!
  if (millis() - lastRead >= 2000) {
    lastRead = millis();
    readSensors();
  }
}
