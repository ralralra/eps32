/*
  온습도(DHT) 셀 — 교실의 공기를 숫자로
  연결: Digital I/O 블록 아무 포트 (그림: images/wiring_digital_cell.png)
  라이브러리: DHT sensor library (Adafruit)
  ⚠ DHT11은 2초에 1번만 측정 — delay(2000) 필수! 값이 nan이면 배선·핀·간격 확인
*/

#include <DHT.h>

#define DHTPIN 27  // '6' 칸 예시 (8→12 · 7→14 · 6→27 · 5→16)
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();  // ℃
  float h = dht.readHumidity();     // %
  Serial.print(t);
  Serial.print("C  ");
  Serial.print(h);
  Serial.println("%");
  delay(2000);  // 센서에 입김을 불어 보세요 — 습도 급상승!
}
