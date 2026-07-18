/*
  2회차 · 따라하기 — 초음파 센서: 박쥐처럼 거리 재기
  소리를 쏘고(TRIG), 메아리가 돌아오는 시간(ECHO)으로 거리 계산.
  자로 실측해서 비교해 보면 신뢰가 생겨요!
*/

#define TRIG 5  // 발사 ★핀맵 카드 확인
#define ECHO 4  // 수신 ★핀맵 카드 확인

long readDistance() {  // '내 함수' — 만들어 두면 재사용 가능!
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(ECHO, HIGH);  // 왕복 시간 (마이크로초)
  return t * 0.034 / 2;          // 소리 초속 340m, 왕복이니 ÷2 → cm
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  Serial.println(readDistance());
  delay(300);
}
