/*
  2회차 · 미션 ② — 가까이 오면 경보! (예시 답안)
  10cm 이내 접근 → LCD 'WARNING!!' + LED 점멸 / 멀어지면 'SAFE'
  심화(3단계)는 if ~ else if ~ else 구조를 참고하세요.
*/

#include <LiquidCrystal_I2C.h>

#define LED  2
#define TRIG 5  // ★핀맵 카드 확인
#define ECHO 4  // ★핀맵 카드 확인

LiquidCrystal_I2C lcd(0x20, 16, 2);

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
  pinMode(LED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Distance Alarm");
}

void loop() {
  long d = readDistance();

  if (d < 10) {              // 10cm 이내? 기준값을 바꾸면 '민감도'가 바뀌어요
    lcd.setCursor(0, 1);
    lcd.print("WARNING!!    ");
    digitalWrite(LED, HIGH);
  }
  else {                     // 아니라면
    lcd.setCursor(0, 1);
    lcd.print("SAFE         ");
    digitalWrite(LED, LOW);
  }
  delay(200);
}
