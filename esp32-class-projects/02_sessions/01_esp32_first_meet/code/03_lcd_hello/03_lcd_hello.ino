/*
  1회차 · 따라하기 — LCD에 첫 글자 표시
  보드: Wemos D1 R32 / LCD 셀은 실드의 I2C(SDA·SCL) 포트에 연결
  라이브러리: LiquidCrystal I2C (라이브러리 관리에서 설치)
  ⚠ 글자가 안 보이면: 주소 0x27 → 0x3F 시도, 뒷면 파란 다이얼(대비) 조절
*/

#include <LiquidCrystal_I2C.h>

// 주소 0x27 (안 되면 0x3F 시도), 16칸 × 2줄
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();          // LCD 준비
  lcd.backlight();     // 백라이트 켜기
  lcd.setCursor(0, 0); // 0칸, 0줄 (첫째 줄)
  lcd.print("Hello Team1!");
  lcd.setCursor(0, 1); // 둘째 줄
  lcd.print("IoT Start!");
}

void loop() { }  // 비어 있어도 화면은 유지돼요
