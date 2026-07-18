/*
  5단계 — 출석 버튼 데이터를 ESP32로 보내기
  ----------------------------------------------------
  목표: 폰에서 이름 넣고 출석 버튼 → 그 이름이 ESP32로 전달되고,
        시리얼 모니터에 "출석: 홍길동" 이 찍힌다.

  새로 배우는 것: 폰(웹페이지)이 ESP32에게 데이터를 "보내는" 방법 = fetch
*/

#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"            // ↓ Brownout(전원 순간강하) 오작동 방지용
#include "soc/rtc_cntl_reg.h"

const char* AP_SSID = "ESP32-Attend";
const char* AP_PASS = "attend1234";

WebServer server(80);

// 출석 버튼을 누르면 fetch('/check?name=...') 로 ESP32에 이름을 보낸다
const char* page = R"HTML(
<!doctype html>
<html lang="ko">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>출석 체크</title>
  <style>
    body { font-family: sans-serif; max-width: 400px; margin: 0 auto; padding: 2rem; text-align: center; }
    h1 { color: #2563eb; }
    input { font-size: 1.2rem; padding: 0.6rem; width: 80%; box-sizing: border-box; }
    button { font-size: 1.2rem; padding: 0.7rem 2rem; margin-top: 1rem;
             background: #2563eb; color: white; border: none; border-radius: 8px; }
    #msg { font-size: 1.2rem; color: #16a34a; margin-top: 1rem; }
  </style>
</head>
<body>
  <h1>📋 우리 반 출석</h1>
  <p>이름을 입력하고 출석 버튼을 누르세요</p>
  <input id="name" placeholder="이름">
  <br>
  <button onclick="go()">출석하기</button>
  <p id="msg"></p>

  <script>
    function go() {
      var name = document.getElementById("name").value;
      if (name === "") { alert("이름을 입력하세요"); return; }
      // ESP32의 /check 주소로 이름을 보낸다
      fetch("/check?name=" + encodeURIComponent(name))
        .then(function(r) { return r.text(); })
        .then(function(answer) {
          document.getElementById("msg").innerText = answer;  // ESP32의 답을 표시
          document.getElementById("name").value = "";
        });
    }
  </script>
</body>
</html>
)HTML";

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", page);
}

// 폰이 /check?name=홍길동 으로 접속하면 실행됨
void handleCheck() {
  String name = server.arg("name");   // 주소에서 name 값 꺼내기
  Serial.print("출석: ");
  Serial.println(name);
  // 폰에게 답장 보내기
  server.send(200, "text/plain; charset=utf-8", name + "님 출석 완료!");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Brownout 감지기 끄기(안전장치)
  Serial.begin(115200);
  delay(300);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println();
  Serial.println("=== ESP32 출석 서버 ===");
  Serial.print("접속 주소: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/check", handleCheck);   // ★ 출석 처리 주소 추가
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();
}
