/*
  1단계 ② 온·습도 센서(DHT11) — 스마트팜 속 공기 상태
  ⚠ 키트 자료에는 A0로 나오지만, D1 R32에서 A0 자리(GPIO2)는 부팅 문제를 일으킬 수 있어요.
     케이블을 확장쉴드의 **D2 줄**에 꽂으세요 → GPIO26
  결선: G(D2)→GND · V(D2)→VCC · S(D2)→DATA
  라이브러리: DHT sensor library (Adafruit)
*/

#include <DHT.h>       // 온습도 센서 제어용 라이브러리 불러오기

#define DHTPIN 26      // DHT11 센서가 연결된 핀 (확장쉴드 D2 줄 = GPIO26)

DHT dht(DHTPIN, DHT11);   // 온습도 센서 객체 생성 (핀 번호, 센서 종류)

// 처음 한 번만 실행
void setup() {
  Serial.begin(115200);   // 시리얼 통신 시작
  dht.begin();            // 온습도 센서 작동 시작
}

// 무한 반복
void loop() {
  float t = dht.readTemperature();   // 섭씨 온도 읽기 (float = 소수점 있는 숫자)
  float h = dht.readHumidity();      // 습도(%) 읽기

  // isnan = "숫자가 아님(Not a Number)" 검사 — 측정에 실패하면 nan이 나와요
  if (isnan(t) || isnan(h)) {
    Serial.println("측정 실패 — 배선(G·V·S)과 핀 번호를 확인하세요");
  } else {
    Serial.printf("온도 %.1f ℃  /  습도 %.0f %%\n", t, h);  // 온도는 소수점 1자리, 습도는 정수로 출력
  }
  delay(2000);   // DHT11은 느린 센서라 2초에 한 번이 적당해요
}
