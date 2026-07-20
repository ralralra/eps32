/*
  1회차 · 종합 미션 ③ — 카운트다운 타이머 (예시 답안)
  LCD에 10 → 0 카운트다운, 0이 되면 'TIME OVER!' + LED 빠르게 5번 점멸
  심화: 빛센서를 가리면(어두우면) 타이머 일시정지!
*/

#include <LiquidCrystal_I2C.h>

#define LED   2    // 내장 LED
#define LIGHT 34   // 빛센서 (A2~A5 위치 — 핀맵 카드 확인)
#define DARK  1000 // '가려짐' 기준값 — 시리얼로 관찰해서 우리 교실에 맞게 조절

LiquidCrystal_I2C lcd(0x20, 16, 2);

void setup() {
  pinMode(LED, OUTPUT);
  lcd.init();
  lcd.backlight();

  for (int i = 10; i >= 0; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timer:");
    lcd.setCursor(0, 1);
    lcd.print(i);
    delay(1000);

    // 심화: 가려져 있는 동안은 기다리기(일시정지)
    while (analogRead(LIGHT) < DARK) {
      lcd.setCursor(8, 0);
      lcd.print("PAUSE");
      delay(200);
    }
    lcd.setCursor(8, 0);
    lcd.print("     ");  // PAUSE 글자 지우기
  }

  // 0이 된 순간!
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TIME OVER!");
  for (int i = 0; i < 5; i++) {   // LED 빠르게 5번 점멸
    digitalWrite(LED, HIGH);
    delay(150);
    digitalWrite(LED, LOW);
    delay(150);
  }
}

void loop() { }
