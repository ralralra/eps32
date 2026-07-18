/*
  빛 셀 — analogRead 기본
  연결: Analog 블록 A2~A5 위치 (그림: images/wiring_analog_cell.png)
  ★ 꽂은 자리의 GPIO로 수정: A2=35 · A3=34 · A4=36 · A5=39
*/

#define LIGHT 34  // A3 자리 예시

void setup() {
  Serial.begin(115200);
}

void loop() {
  int v = analogRead(LIGHT);  // 0(어두움) ~ 4095(밝음)
  Serial.print("light: ");
  Serial.println(v);
  delay(300);  // 손으로 가려 보세요 — 숫자가 뚝!
}
