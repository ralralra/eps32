/*
  1단계 ① 토양수분 감지 모듈 — 흙의 목마름을 숫자로!
  ⚠ 키트 자료에는 A1로 나오지만, D1 R32에서는 A1 자리(GPIO4)가 WiFi와 겹쳐요.
     케이블을 확장쉴드의 **A2 줄**에 꽂으세요 → GPIO35
  결선: G(A2)→GND · V(A2)→VCC · S(A2)→AO
  주의: 센서의 전원부(위쪽 기판)에 물이 닿지 않게!
*/

#define SOIL 35        // A2 자리 (D1 R32에서 GPIO35)
#define SOIL_MAX 4095  // 물 흠뻑 준 화분에서 측정한 최댓값으로 바꾸세요! (아래 순서 참고)

void setup() {
  Serial.begin(115200);
}

void loop() {
  int raw = analogRead(SOIL);                    // 0 ~ 4095 (우노는 0~1023이었어요!)
  int pct = map(raw, 0, SOIL_MAX, 0, 100);       // 0 ~ 100% 로 변환
  Serial.printf("아날로그 %d  →  토양습도 %d%%\n", raw, pct);
  delay(500);
}

/*
  [최댓값 찾는 순서 — 키트 자료 방식 그대로]
  1. 물을 충분히 준 화분에 센서를 꽂고 시리얼 모니터의 '아날로그' 값 관찰
  2. 그 최댓값을 SOIL_MAX에 입력 → 이제 %가 정확해져요
  3. 마른 흙 / 젖은 흙의 % 차이를 관찰해 보세요
*/
