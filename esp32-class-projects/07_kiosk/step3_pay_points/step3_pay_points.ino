/*
  3단계 — 가상 포인트 모의결제 + 판매 기록 (ESP32 기기에서)
  ----------------------------------------------------
  목표: 포인트 잔액과 판매 기록을 ESP32(SPIFFS)에 둔다.
        결제하면 기기의 포인트가 줄고, 판매가 기록된다.
        어느 폰으로 접속하든 같은 잔액/기록을 공유한다.

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

// ── 포인트 잔액 읽기/쓰기 ──
int getBalance() {
  File f = SPIFFS.open("/balance.txt", FILE_READ);
  if (!f) return START_POINTS;          // 처음이면 시작 포인트
  int b = f.readString().toInt();
  f.close();
  return b;
}
void setBalance(int b) {
  File f = SPIFFS.open("/balance.txt", FILE_WRITE);
  if (f) { f.print(b); f.close(); }
}

// 판매 1건(상품,가격)을 기록
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
</style></head><body>
<h1>🍪 과자 키오스크</h1>
<div class="points">남은 포인트: <b id="points">...</b>P</div>
<div class="menu" id="menu"></div>
<div class="cart"><ul id="cart"></ul>
<div class="total">합계: <span id="total">0</span>P</div>
<button class="pay" onclick="pay()">결제하기</button><div id="msg"></div></div>
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
 // 장바구니를 "새우깡:700|콜라:1000" 형태로 만들어 ESP32에 보냄
 var s=cart.map(function(p){return p.name+":"+p.price;}).join("|");
 fetch("/pay?cart="+encodeURIComponent(s)).then(r=>r.text()).then(function(res){
   var parts=res.split(",");
   if(parts[0]==="OK"){msg("결제 완료! 잔액 "+parts[1]+"P","#16a34a");cart=[];drawCart();loadBalance();}
   else{msg("포인트가 부족합니다!","#dc2626");}
 });}
function msg(x,c){var m=document.getElementById("msg");m.innerText=x;m.style.color=c;}
drawMenu();loadBalance();
</script></body></html>
)HTML";

void handleRoot()    { server.send(200, "text/html; charset=utf-8", page); }
void handleBalance() { server.send(200, "text/plain", String(getBalance())); }

// 결제 처리: cart="새우깡:700|콜라:1000"
void handlePay() {
  String cart = server.arg("cart");
  // 합계 계산하면서 항목 분리
  int total = 0;
  int start = 0;
  // 먼저 합계만 구함
  String tmp = cart;
  while (tmp.length() > 0) {
    int bar = tmp.indexOf('|');
    String one = (bar < 0) ? tmp : tmp.substring(0, bar);
    int colon = one.indexOf(':');
    if (colon > 0) total += one.substring(colon + 1).toInt();
    if (bar < 0) break;
    tmp = tmp.substring(bar + 1);
  }

  int balance = getBalance();
  if (balance < total) {
    server.send(200, "text/plain", "LOW");
    return;
  }

  // 잔액 차감 + 각 항목 기록
  setBalance(balance - total);
  tmp = cart;
  while (tmp.length() > 0) {
    int bar = tmp.indexOf('|');
    String one = (bar < 0) ? tmp : tmp.substring(0, bar);
    int colon = one.indexOf(':');
    if (colon > 0) logItem(one.substring(0, colon), one.substring(colon + 1).toInt());
    if (bar < 0) break;
    tmp = tmp.substring(bar + 1);
  }
  Serial.printf("결제 %d원, 잔액 %d원\n", total, balance - total);
  server.send(200, "text/plain", "OK," + String(balance - total));
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(300);
  if (!SPIFFS.begin(true)) Serial.println("SPIFFS 실패!");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("키오스크: http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/balance", handleBalance);
  server.on("/pay", handlePay);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() { server.handleClient(); }
