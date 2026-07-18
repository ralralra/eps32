/*
  홀(자석) 셀 — 문 열림 감지 (출석·안전 프로젝트의 핵심!)
  연결: Digital I/O 블록 아무 포트 (그림: images/wiring_digital_cell.png)
  사용법: 셀 옆에 자석을 가까이/멀리 — 문틀에 셀, 문에 자석을 붙이면 도어 센서!
*/

#define HALL 27  // '6' 칸 예시 (8→12 · 7→14 · 6→27 · 5→16)
#define LED  2

void setup() {
  Serial.begin(115200);
  pinMode(HALL, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  int v = digitalRead(HALL);  // 자석 감지 시 값이 바뀜 (셀에 따라 0 또는 1)

  if (v == 1) {               // ★ 우리 셀 기준으로 조건 확인 후 조정
    Serial.println("문 열림!  (자석 멀어짐)");
    digitalWrite(LED, HIGH);
  } else {
    Serial.println("문 닫힘   (자석 감지)");
    digitalWrite(LED, LOW);
  }
  delay(200);
}
