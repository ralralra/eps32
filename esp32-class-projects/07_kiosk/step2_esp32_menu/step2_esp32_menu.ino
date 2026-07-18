/*
  2단계 — ESP32가 키오스크 화면 보여주기
  ----------------------------------------------------
  목표: 폰/태블릿으로 ESP32에 접속하면 1단계 키오스크 화면이 뜬다.
        상품 선택·장바구니·합계까지 동작 (결제는 아직 브라우저 안에서).

  3단계에서 결제를 ESP32(기기)로 옮긴다.
*/

#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* AP_SSID = "ESP32-Kiosk";
const char* AP_PASS = "kiosk1234";
WebServer server(80);

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
<div class="points">남은 포인트: <b id="points">10000</b>P</div>
<div class="menu" id="menu"></div>
<div class="cart"><ul id="cart"></ul>
<div class="total">합계: <span id="total">0</span>P</div>
<button class="pay" onclick="pay()">결제하기</button><div id="msg"></div></div>
<script>
var products=[{name:"새우깡",price:700},{name:"초코파이",price:500},{name:"콜라",price:1000},{name:"사탕",price:200}];
var cart=[];
function getPoints(){return parseInt(localStorage.getItem("points")||"10000");}
function setPoints(p){localStorage.setItem("points",p);document.getElementById("points").innerText=p;}
function drawMenu(){var m=document.getElementById("menu");m.innerHTML="";products.forEach(function(p,i){
 var d=document.createElement("div");d.className="item";d.innerHTML="<b>"+p.name+"</b><br>"+p.price+"P";
 d.onclick=function(){cart.push(products[i]);drawCart();};m.appendChild(d);});}
function drawCart(){var ul=document.getElementById("cart");ul.innerHTML="";var t=0;
 cart.forEach(function(p){var li=document.createElement("li");li.textContent=p.name+" - "+p.price+"P";ul.appendChild(li);t+=p.price;});
 document.getElementById("total").innerText=t;return t;}
function pay(){var t=drawCart();if(t===0){msg("상품을 담아주세요","#d97706");return;}
 var pt=getPoints();if(pt<t){msg("포인트가 부족합니다!","#dc2626");return;}
 setPoints(pt-t);msg("결제 완료! "+t+"P 사용","#16a34a");cart=[];drawCart();}
function msg(x,c){var m=document.getElementById("msg");m.innerText=x;m.style.color=c;}
drawMenu();setPoints(getPoints());
</script></body></html>
)HTML";

void handleRoot() { server.send(200, "text/html; charset=utf-8", page); }

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  delay(300);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("키오스크: http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.begin();
  Serial.println("서버 시작됨!");
}

void loop() { server.handleClient(); }
