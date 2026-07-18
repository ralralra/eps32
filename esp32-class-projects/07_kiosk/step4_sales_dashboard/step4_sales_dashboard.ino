/*
  4단계 — 매출 집계 대시보드 (완성본)
  ----------------------------------------------------
  목표: 쌓인 판매 기록으로 총매출/상품별 판매수를 집계해서 보여준다.

  손님 화면:   http://192.168.4.1/        (키오스크)
  사장님 화면: http://192.168.4.1/sales   (매출 집계)
  ※ Partition: Default 4MB with spiffs
*/

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* AP_SSID = "ESP32-Kiosk";
const char* AP_PASS = "kiosk1234";
WebServer server(80);

const int START_POINTS = 10000;

// 집계용 상품 목록 (이름은 키오스크 화면의 products와 일치)
const int P = 4;
String pName[P]  = { "새우깡", "초코파이", "콜라", "사탕" };

int getBalance() {
  File f = SPIFFS.open("/balance.txt", FILE_READ);
  if (!f) return START_POINTS;
  int b = f.readString().toInt(); f.close(); return b;
}
void setBalance(int b) {
  File f = SPIFFS.open("/balance.txt", FILE_WRITE);
  if (f) { f.print(b); f.close(); }
}
void logItem(String name, int price) {
  File f = SPIFFS.open("/sales.csv", FILE_APPEND);
  if (f) { f.printf("%s,%d\n", name.c_str(), price); f.close(); }
}

const char* page = R"HTML(
<!doctype html><html lang="ko"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>과자 키오스크</title><style>
body{font-family:sans-serif;max-width:500px;margin:0 auto;padding:1rem}
h1{color:#7c3aed;text-align:center}
.points{text-align:center;font-size:1.2rem;margin-bottom:1rem}
.menu{display:grid;grid-template-columns:1fr 1fr;gap:.6rem}
.item{padding:1rem;border:2px solid #7c3aed;border-radius:10px;text-align:center;cursor:pointer}
.item:active{background:#ede9fe}
.cart{margin-top:1rem;border-top:2px dashed #ccc;padding-top:.5rem}
.total{font-size:1.4rem;font-weight:bold;text-align:right}
button.pay{width:100%;font-size:1.3rem;padding:.8rem;margin-top:.5rem;background:#7c3aed;color:#fff;border:none;border-radius:10px}
#msg{text-align:center;font-size:1.1rem;color:#16a34a;min-height:1.5rem}
a{display:block;text-align:center;margin-top:1.5rem;color:#888}
</style></head><body>
<h1>🍪 과자 키오스크</h1>
<div class="points">남은 포인트: <b id="points">...</b>P</div>
<div class="menu" id="menu"></div>
<div class="cart"><ul id="cart"></ul>
<div class="total">합계: <span id="total">0</span>P</div>
<button class="pay" onclick="pay()">결제하기</button><div id="msg"></div></div>
<a href="/sales">(사장님) 매출 보기</a>
<script>
var products=[{name:"새우깡",price:700},{name:"초코파이",price:500},{name:"콜라",price:1000},{name:"사탕",price:200}];
var cart=[];
function loadBalance(){fetch("/balance").then(r=>r.text()).then(t=>{document.getElementById("points").innerText=t;});}
function drawMenu(){var m=document.getElementById("menu");m.innerHTML="";products.forEach(function(p,i){
 var d=document.createElement("div");d.className="item";d.innerHTML="<b>"+p.name+"</b><br>"+p.price+"P";
 d.onclick=function(){cart.push(products[i]);drawCart();};m.appendChild(d);});}
function drawCart(){var ul=document.getElementById("cart");ul.innerHTML="";var t=0;
 cart.forEach(function(p){var li=document.createElement("li");li.textContent=p.name+" - "+p.price+"P";ul.appendChild(li);t+=p.price;});
 document.getElementById("total").innerText=t;return t;}
function pay(){var t=drawCart();if(t===0){msg("상품을 담아주세요","#d97706");return;}
 var s=cart.map(function(p){return p.name+":"+p.price;}).join("|");
 fetch("/pay?cart="+encodeURIComponent(s)).then(r=>r.text()).then(function(res){
   var parts=res.split(",");
   if(parts[0]==="OK"){msg("결제 완료! 잔액 "+parts[1]+"P","#16a34a");cart=[];drawCart();loadBalance();}
   else{msg("포인트가 부족합니다!","#dc2626");}});}
function msg(x,c){var m=document.getElementById("msg");m.innerText=x;m.style.color=c;}
drawMenu();loadBalance();
</script></body></html>
)HTML";

void handleRoot()    { server.send(200, "text/html; charset=utf-8", page); }
void handleBalance() { server.send(200, "text/plain", String(getBalance())); }

int sumCart(String cart, void (*cb)(String, int)) {
  int total = 0;
  String tmp = cart;
  while (tmp.length() > 0) {
    int bar = tmp.indexOf('|');
    String one = (bar < 0) ? tmp : tmp.substring(0, bar);
    int colon = one.indexOf(':');
    if (colon > 0) {
      int price = one.substring(colon + 1).toInt();
      total += price;
      if (cb) cb(one.substring(0, colon), price);
    }
    if (bar < 0) break;
    tmp = tmp.substring(bar + 1);
  }
  return total;
}

void handlePay() {
  String cart = server.arg("cart");
  int total = sumCart(cart, NULL);          // 합계만
  int balance = getBalance();
  if (balance < total) { server.send(200, "text/plain", "LOW"); return; }
  setBalance(balance - total);
  sumCart(cart, logItem);                   // 각 항목 기록
  Serial.printf("결제 %d원, 잔액 %d원\n", total, balance - total);
  server.send(200, "text/plain", "OK," + String(balance - total));
}

// 매출 집계 페이지
void handleSales() {
  int count[P] = {0}; int revenue = 0;
  File f = SPIFFS.open("/sales.csv", FILE_READ);
  if (f) {
    while (f.available()) {
      String line = f.readStringUntil('\n'); line.trim();
      if (line.length() == 0) continue;
      int c = line.indexOf(',');
      if (c < 0) continue;
      String name = line.substring(0, c);
      int price = line.substring(c + 1).toInt();
      revenue += price;
      for (int i = 0; i < P; i++) if (name == pName[i]) count[i]++;
    }
    f.close();
  }

  String html = "<!doctype html><html lang='ko'><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>매출</title><style>body{font-family:sans-serif;padding:1rem;max-width:480px;margin:0 auto}";
  html += "table{border-collapse:collapse;width:100%}th,td{border:1px solid #ccc;padding:8px;text-align:center}";
  html += "th{background:#7c3aed;color:#fff}a{display:inline-block;margin-top:1rem}</style></head><body>";
  html += "<h2>💰 매출 집계</h2>";
  html += "<p>총 매출: <b>" + String(revenue) + "P</b></p>";
  html += "<p>현재 포인트 잔액: " + String(getBalance()) + "P</p>";
  html += "<table><tr><th>상품</th><th>판매 수</th></tr>";
  for (int i = 0; i < P; i++)
    html += "<tr><td>" + pName[i] + "</td><td>" + String(count[i]) + "</td></tr>";
  html += "</table>";
  html += "<a href='/reset' onclick=\"return confirm('판매기록을 초기화할까요?')\">🗑 매출 초기화</a> | ";
  html += "<a href='/'>← 키오스크</a></body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleReset() {
  SPIFFS.remove("/sales.csv");
  server.send(200, "text/html; charset=utf-8",
              "<meta charset='utf-8'>매출 초기화 완료! <a href='/sales'>돌아가기</a>");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(300);
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 실패!");
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("키오스크: http://"); Serial.print(WiFi.softAPIP()); Serial.println("/");
  Serial.print("매출:     http://"); Serial.print(WiFi.softAPIP()); Serial.println("/sales");
  server.on("/", handleRoot);
  server.on("/balance", handleBalance);
  server.on("/pay", handlePay);
  server.on("/sales", handleSales);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() { server.handleClient(); }
