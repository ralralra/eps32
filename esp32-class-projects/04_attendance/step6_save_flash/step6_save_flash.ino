/*
  6단계 — ESP32 안에 출석 기록 저장하기 (전원 꺼도 유지)
  ----------------------------------------------------
  목표: 출석한 이름+시각을 ESP32 내부 저장공간(SPIFFS)에 적어둔다.
        전원을 껐다 켜도 기록이 남는다.

  3단계의 localStorage(브라우저 저장) → 여기선 SPIFFS(보드 저장).
  개념은 똑같다: 꺼내기 → 추가 → 다시 넣기.

  시각: ESP32는 인터넷이 없어 시계가 없다. 그래서 폰의 시각을 같이 받는다.

  ※ Tools → Partition Scheme → "Default 4MB with spiffs" 확인
*/

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "soc/soc.h"            // ↓ Brownout(전원 순간강하) 오작동 방지용
#include "soc/rtc_cntl_reg.h"

const char* AP_SSID = "ESP32-Attend";
const char* AP_PASS = "attend1234";
const char* FILE_PATH = "/attend.csv";   // 출석부 파일

WebServer server(80);

// 폰의 시각도 함께 보낸다: /check?name=홍길동&time=...
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
      var time = new Date().toLocaleString();   // 폰의 현재 시각
      fetch("/check?name=" + encodeURIComponent(name) + "&time=" + encodeURIComponent(time))
        .then(function(r) { return r.text(); })
        .then(function(answer) {
          document.getElementById("msg").innerText = answer;
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

void handleCheck() {
  String name = server.arg("name");
  String time = server.arg("time");

  // ── 저장: 파일 끝에 한 줄 추가 (이름,시각) ──
  File f = SPIFFS.open(FILE_PATH, FILE_APPEND);
  if (f) {
    f.println(name + "," + time);
    f.close();
  }

  Serial.println("출석 저장: " + name + " (" + time + ")");
  server.send(200, "text/plain; charset=utf-8", name + "님 출석 완료!");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Brownout 감지기 끄기(안전장치)
  Serial.begin(115200);
  delay(300);

  // SPIFFS(내부 저장공간) 준비. true = 처음이면 자동 포맷
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 시작 실패!");
  }

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println();
  Serial.println("=== ESP32 출석 서버 ===");
  Serial.print("접속 주소: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/check", handleCheck);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();
}
