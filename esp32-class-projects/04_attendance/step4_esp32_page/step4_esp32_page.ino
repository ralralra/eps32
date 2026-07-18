/*
  4단계 — ESP32가 출석 웹페이지를 보여주기
  ----------------------------------------------------
  목표: 폰으로 ESP32에 접속하면, 1단계에서 만든 출석 페이지가 뜬다.
  지금까진 PC 브라우저로 .html 파일을 직접 열었지만,
  이제는 ESP32가 그 HTML을 "보내주는" 역할을 한다.

  보드: Wemos D1 R32 (툴 → 보드 → ESP32 Dev Module)
  부품: 없음 (USB만 연결)
*/

#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"            // ↓ Brownout(전원 순간강하) 오작동 방지용
#include "soc/rtc_cntl_reg.h"

// ESP32가 직접 만들 와이파이 이름/비밀번호
const char* AP_SSID = "ESP32-Attend";   // 폰 와이파이 목록에 뜰 이름
const char* AP_PASS = "attend1234";      // 8자 이상이어야 함

WebServer server(80);   // 80번 포트로 웹서버

// ── 폰에 보낼 웹페이지 (HTML을 통째로 문자열로 넣음) ──
// R"HTML( ... )HTML"  =  따옴표 신경 안 쓰고 긴 글을 그대로 담는 방법
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
  </style>
</head>
<body>
  <h1>📋 우리 반 출석</h1>
  <p>이름을 입력하고 출석 버튼을 누르세요</p>
  <input id="name" placeholder="이름">
  <br>
  <button onclick="alert('버튼이 눌렸어요! 다음 단계에서 진짜 출석되게 만들어요')">출석하기</button>
  <hr>
  <small>이 페이지는 ESP32가 보냈습니다 😊</small>
</body>
</html>
)HTML";

// 누군가 "/" 주소로 접속하면 → page 를 보내준다
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", page);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Brownout 감지기 끄기(안전장치)
  Serial.begin(115200);
  delay(300);

  // ESP32가 스스로 와이파이를 만든다 (AP 모드)
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println();
  Serial.println("=== ESP32 출석 서버 ===");
  Serial.print("와이파이 이름: ");
  Serial.println(AP_SSID);
  Serial.print("접속 주소:    http://");
  Serial.println(WiFi.softAPIP());   // 보통 192.168.4.1

  // 주소별 처리 등록
  server.on("/", handleRoot);
  server.begin();
  Serial.println("서버 시작됨! 폰으로 위 와이파이에 접속하세요.");
}

void loop() {
  server.handleClient();   // 접속 요청을 계속 처리
}
