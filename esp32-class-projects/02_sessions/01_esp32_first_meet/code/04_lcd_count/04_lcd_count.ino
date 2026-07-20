/*
  1회차 · 따라하기 — LCD 활용 3기술: 글자·숫자·갱신
  변하는 숫자를 1초마다 화면에 표시 — 카운트다운 타이머의 심장!
  count가 0 밑으로 내려가면 어떻게 될까요? 직접 관찰해 봐요.
*/

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x20, 16, 2);

int count = 10;

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  lcd.clear();          // 화면 지우기
  lcd.setCursor(0, 0);
  lcd.print("Count:");
  lcd.setCursor(0, 1);
  lcd.print(count);     // 숫자도 표시!
  count = count - 1;    // 1씩 감소
  delay(1000);
}
