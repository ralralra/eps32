/*
  2회차 · 따라하기 ① — 진동(디지털)과 빛(아날로그) 읽기
  시리얼 모니터(115200 baud)로 관찰: 책상을 두드리면 vib가 튀고,
  센서를 가리면 light가 뚝 떨어져요.

  배선: 진동 셀 → 디지털 포트 (★핀맵 카드 확인)
       빛 셀 → A2~A5 위치 포트 (★핀맵 카드 확인)
*/

#define VIB   27  // 진동 ★핀맵 카드
#define LIGHT 34  // 빛 (A2~A5 위치)

void setup() {
  Serial.begin(115200);  // 보드↔PC 대화 채널 열기
  pinMode(VIB, INPUT);
}

void loop() {
  int v = digitalRead(VIB);   // 0 or 1
  int l = analogRead(LIGHT);  // 0~4095
  Serial.print("vib:");
  Serial.print(v);
  Serial.print(" light:");
  Serial.println(l);
  delay(300);
}
