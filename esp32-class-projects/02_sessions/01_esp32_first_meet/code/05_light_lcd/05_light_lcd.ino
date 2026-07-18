/*
  1회차 · 따라하기 — 첫 센서: 빛센서 값을 LCD에 표시
  센서 → 숫자 → 화면, 3단 흐름이 IoT의 최소 단위!

  배선: 빛센서 셀 → 실드의 A2~A5 위치 포트 (★실제 GPIO 번호는 포트-핀맵 카드 확인)
       LCD 셀 → I2C 포트
  ⚠ 아날로그 셀이 A2~A5(GPIO 34·35·36·39)여야 하는 이유:
     WiFi를 켜면 다른 자리(ADC2)는 아날로그 읽기가 멈춰요. (3회차에 체감!)
*/

#include <LiquidCrystal_I2C.h>

#define LIGHT 34  // 빛센서 포트 (A2~A5 위치 — 핀맵 카드 확인)

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  int v = analogRead(LIGHT);  // 0(어두움) ~ 4095(밝음)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  lcd.print(v);
  delay(500);
}
