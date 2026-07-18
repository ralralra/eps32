/*
  진동 셀 — 충격·움직임 감지 (디지털 0/1)
  연결: Digital I/O 블록 아무 포트 (그림: images/wiring_digital_cell.png)
  ★ 칸 라벨→GPIO: 8→12 · 7→14 · 6→27 · 5→16
*/

#define VIB 27  // '6' 칸 예시
#define LED 2   // 내장 LED — 충격 오면 켜기

void setup() {
  Serial.begin(115200);
  pinMode(VIB, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  int v = digitalRead(VIB);  // 0 또는 1
  digitalWrite(LED, v);      // 충격 순간 LED 반짝
  Serial.print("vib: ");
  Serial.println(v);
  delay(100);  // 책상을 톡톡 두드려 보세요!
}
