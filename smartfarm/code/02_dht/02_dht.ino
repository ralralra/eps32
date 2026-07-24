/*
  1단계 ② 온·습도 센서(DHT11) — 스마트팜 속 공기 상태
  ⚠ 키트 자료에는 A0로 나오지만, D1 R32에서 A0 자리(GPIO2)는 부팅 문제를 일으킬 수 있어요.
     케이블을 확장쉴드의 **D2 줄**에 꽂으세요 → GPIO26
  결선: G(D2)→GND · V(D2)→VCC · S(D2)→DATA
  라이브러리: DHT sensor library (Adafruit)
*/

#include <DHT.h>

#define DHTPIN 26      // D2 자리 (D1 R32에서 GPIO26)

DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("측정 실패 — 배선(G·V·S)과 핀 번호를 확인하세요");
  } else {
    Serial.printf("온도 %.1f ℃  /  습도 %.0f %%\n", t, h);
  }
  delay(2000);   // DHT11은 2초에 한 번이 적당해요
}
