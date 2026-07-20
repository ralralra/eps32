/*
  3회차 · 종합 미션 ② — 내 손안의 리모컨 (완성 예시)
  폰 한 화면에서: LED 켜기/끄기 + 숫자를 교실 7세그로 전송 + 온습도 실시간 표시
  팀원 폰 2대 이상 동시 접속 → 서로 7세그에 번호를 보내 봐요!
  (7회차 키오스크의 '주문번호 표시'가 바로 이 원리예요)

  배선: 7세그 → '6'·'7' 칸 (DIO=27·CLK=14)
       온습도(DHT) 셀 → PWM 구역 스티커 23 포트 (옆라벨 '11'은 우노 라벨!)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <TM1637Display.h>
#include <DHT.h>

#define LED    2   // 내장 LED
#define DHTPIN 23  // 온습도 (스티커 23 포트)
#define DIO    27  // '6' 칸
#define CLK    14  // '7' 칸

TM1637Display display(CLK, DIO);
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
    input { font-size: 20px; padding: 12px; border-radius: 8px; border: 1px solid #aaa; width: 160px; }
    p { font-size: 24px; }
  </style>
</head>
<body>
  <h1>Team1 리모컨</h1>

  <p>온도 <span id="t">--</span>℃ · 습도 <span id="h">--</span>%</p>

  <button onclick="fetch('/led/on')">LED 켜기</button>
  <button onclick="fetch('/led/off')">LED 끄기</button>

  <p>
    <input id="num" type="number" placeholder="7세그로 보낼 숫자">
    <button onclick="sendNum()">보내기</button>
  </p>

  <script>
    function sendNum() {
      const n = document.getElementById("num").value;
      fetch('/seg?n=' + n);
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
  display.setBrightness(7);
  display.showNumberDec(0);

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
  server.on("/seg", []() {
    int n = server.arg("n").toInt();   // 주소 뒤 ?n=... 에서 숫자 꺼내기
    display.showNumberDec(n);          // 7세그는 0~9999까지 표시!
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
