/*
  3회차 · 종합 미션 ② — 내 손안의 리모컨 (완성 예시)
  폰 한 화면에서: LED 켜기/끄기 + 글자를 교실 LCD로 전송 + 온습도 실시간 표시
  팀원 폰 2대 이상 동시 접속 → 서로 LCD에 인사를 보내 봐요!

  배선: LCD 셀 → I2C 포트 / DHT 셀 → 디지털 포트 (★핀맵 카드 확인)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define LED    2
#define DHTPIN 19  // ★핀맵 카드 확인

LiquidCrystal_I2C lcd(0x20, 16, 2);
DHT dht(DHTPIN, DHT11);
WebServer server(80);

const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>내 손안의 리모컨</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #EDF3FE; }
    h1 { color: #2E6BE6; }
    button {
      font-size: 22px; padding: 16px 32px; margin: 8px;
      border-radius: 12px; border: none; background: #2E6BE6; color: white;
    }
    input { font-size: 20px; padding: 12px; border-radius: 8px; border: 1px solid #aaa; }
    p { font-size: 24px; }
  </style>
</head>
<body>
  <h1>Team1 리모컨</h1>

  <p>온도 <span id="t">--</span>℃ · 습도 <span id="h">--</span>%</p>

  <button onclick="fetch('/led/on')">LED 켜기</button>
  <button onclick="fetch('/led/off')">LED 끄기</button>

  <p>
    <input id="msg" placeholder="LCD로 보낼 글자(영문)">
    <button onclick="sendMsg()">보내기</button>
  </p>

  <script>
    function sendMsg() {
      const m = document.getElementById("msg").value;
      fetch('/lcd?msg=' + encodeURIComponent(m));
    }
    async function update() {
      const res = await fetch("/sensor");
      const d = await res.json();
      document.getElementById("t").innerText = d.temp;
      document.getElementById("h").innerText = d.humi;
    }
    setInterval(update, 2000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  pinMode(LED, OUTPUT);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Remote Ready!");

  WiFi.softAP("Team1_IoT", "12345678");

  server.on("/", []() {
    server.send(200, "text/html", PAGE);
  });
  server.on("/led/on", []() {
    digitalWrite(LED, HIGH);
    server.send(200, "text/plain", "OK");
  });
  server.on("/led/off", []() {
    digitalWrite(LED, LOW);
    server.send(200, "text/plain", "OK");
  });
  server.on("/lcd", []() {
    String msg = server.arg("msg");  // 주소 뒤 ?msg=... 에서 글자 꺼내기
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msg.substring(0, 16)); // LCD는 한 줄 16자 (한글 불가!)
    server.send(200, "text/plain", "OK");
  });
  server.on("/sensor", []() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    String json = "{\"temp\":" + String(t) + ",\"humi\":" + String(h) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
