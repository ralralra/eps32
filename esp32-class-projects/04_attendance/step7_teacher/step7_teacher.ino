/*
  7단계 — 선생님용 전체 출석부 보기 + CSV 다운로드 (완성본)
  ----------------------------------------------------
  목표: 선생님이 /list 주소로 접속하면 전체 출석 명단을 표로 보고,
        CSV로 내려받거나 전체 초기화할 수 있다.

  이 파일이 출석 프로젝트의 "완성본" 이다.
    학생 화면:   http://192.168.4.1/         (이름 넣고 출석)
    선생님 화면: http://192.168.4.1/list     (전체 명단/다운로드/초기화)

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
const char* FILE_PATH = "/attend.csv";

WebServer server(80);

// ── 학생용 출석 페이지 ──
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
    a { display:inline-block; margin-top: 2rem; color:#888; font-size:0.9rem; }
  </style>
</head>
<body>
  <h1>📋 우리 반 출석</h1>
  <p>이름을 입력하고 출석 버튼을 누르세요</p>
  <input id="name" placeholder="이름">
  <br>
  <button onclick="go()">출석하기</button>
  <p id="msg"></p>
  <a href="/list">(선생님) 전체 출석부 보기</a>

  <script>
    function go() {
      var name = document.getElementById("name").value;
      if (name === "") { alert("이름을 입력하세요"); return; }
      var time = new Date().toLocaleString();
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
  File f = SPIFFS.open(FILE_PATH, FILE_APPEND);
  if (f) {
    f.println(name + "," + time);
    f.close();
  }
  Serial.println("출석 저장: " + name + " (" + time + ")");
  server.send(200, "text/plain; charset=utf-8", name + "님 출석 완료!");
}

// ── 선생님용: 전체 명단을 표로 보여주기 ──
void handleList() {
  String html = "<!doctype html><html lang='ko'><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>출석부</title><style>";
  html += "body{font-family:sans-serif;padding:1rem}";
  html += "table{border-collapse:collapse;width:100%}";
  html += "th,td{border:1px solid #ccc;padding:8px;text-align:center}";
  html += "th{background:#2563eb;color:#fff}";
  html += "a{display:inline-block;margin:1rem 0.5rem 0 0}";
  html += "</style></head><body>";
  html += "<h2>📋 출석부</h2>";
  html += "<table><tr><th>번호</th><th>이름</th><th>시각</th></tr>";

  int count = 0;
  File f = SPIFFS.open(FILE_PATH, FILE_READ);
  if (f) {
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      int comma = line.indexOf(',');
      String name = (comma >= 0) ? line.substring(0, comma) : line;
      String time = (comma >= 0) ? line.substring(comma + 1) : "";
      count++;
      html += "<tr><td>" + String(count) + "</td><td>" + name + "</td><td>" + time + "</td></tr>";
    }
    f.close();
  }
  html += "</table>";
  html += "<p>총 <b>" + String(count) + "</b>명 출석</p>";
  html += "<a href='/download'>📥 CSV 다운로드</a>";
  html += "<a href='/reset' onclick=\"return confirm('정말 전체 삭제할까요?')\">🗑 전체 초기화</a>";
  html += "<a href='/'>← 출석 화면</a>";
  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

// ── 선생님용: CSV 파일 그대로 내려받기 ──
void handleDownload() {
  File f = SPIFFS.open(FILE_PATH, FILE_READ);
  if (!f) {
    server.send(200, "text/plain; charset=utf-8", "아직 출석 기록이 없습니다.");
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=attend.csv");
  server.streamFile(f, "text/csv");
  f.close();
}

// ── 선생님용: 전체 초기화 ──
void handleReset() {
  SPIFFS.remove(FILE_PATH);
  Serial.println("출석부 초기화됨");
  server.send(200, "text/html; charset=utf-8",
              "<meta charset='utf-8'>초기화 완료! <a href='/list'>돌아가기</a>");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Brownout 감지기 끄기(안전장치)
  Serial.begin(115200);
  delay(300);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 시작 실패!");
  }

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println();
  Serial.println("=== ESP32 출석 서버 (완성본) ===");
  Serial.print("학생 화면:   http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/");
  Serial.print("선생님 화면: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/list");

  server.on("/", handleRoot);
  server.on("/check", handleCheck);
  server.on("/list", handleList);
  server.on("/download", handleDownload);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() {
  server.handleClient();
}
