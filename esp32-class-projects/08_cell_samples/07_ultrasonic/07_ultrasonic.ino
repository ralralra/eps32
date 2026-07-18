/*
  초음파 셀 — 박쥐처럼 거리 재기
  연결: Digital 포트 1개의 신호 2칸 통째로 (그림: images/wiring_ultrasonic.png)
       위 칸 = TRIG, 아래 칸 = ECHO (예: '6'·'7' 칸 → GPIO 27·14)
  ⚠ 항상 0이 나오면 TRIG·ECHO가 바뀐 것!
*/

#define TRIG 27  // '6' 칸
#define ECHO 14  // '7' 칸

long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(ECHO, HIGH);  // 왕복 시간
  return t * 0.034 / 2;          // cm (소리 초속 340m, 왕복 ÷2)
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  Serial.print(readDistance());
  Serial.println(" cm");
  delay(300);  // 자로 실측해서 비교해 보세요!
}
