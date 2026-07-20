/*
  2회차 · 미션 ② — 가까이 오면 경보! (예시 답안)
  7세그에 현재 거리(cm)가 계속 표시되고,
  10cm 이내 접근 → LED 점멸 + 시리얼 WARNING!! / 멀어지면 SAFE
  심화(3단계)는 if ~ else if ~ else 구조를 참고하세요.

  배선: 초음파 → '6'·'7' 칸 (TRIG=27·ECHO=14)
       7세그   → '2'·'3' 칸 (DIO=26·CLK=25)
*/

#include <TM1637Display.h>

#define LED  2   // 내장 LED
#define TRIG 27  // '6' 칸
#define ECHO 14  // '7' 칸
#define DIO  26  // '2' 칸
#define CLK  25  // '3' 칸

TM1637Display display(CLK, DIO);

long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(ECHO, HIGH);
  return t * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  display.setBrightness(7);
}

void loop() {
  long d = readDistance();
  display.showNumberDec(d);    // 거리(cm)를 7세그에

  if (d < 10) {                // 10cm 이내? 기준값을 바꾸면 '민감도'가 바뀌어요
    Serial.println("WARNING!!");
    digitalWrite(LED, HIGH);
  }
  else {                       // 아니라면
    Serial.println("SAFE");
    digitalWrite(LED, LOW);
  }
  delay(200);
}
