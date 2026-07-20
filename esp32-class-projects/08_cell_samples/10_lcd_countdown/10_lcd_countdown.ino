/*
  (옵션) LCD 카운트다운 타이머 — 본 과정 미사용! 수업용은 06_countdown_timer(7세그)·09_7segment_timer를 쓰세요.

  ⚠ 이 실드 + D1 R32 조합에서는 I2C 블록 경유 LCD가 동작하지 않을 수 있어요
    (실드가 우노 A4·A5 라인을 쓰면 D1 R32에선 입력 전용 핀 36·39가 되기 때문).
    쓰려면: LCD를 디지털 '6'·'7' 칸에 꽂고 (SDA→'6'칸, SCL→'7'칸)
    아래 Wire.begin(27, 14)이 그 배선 기준이에요. 주소 확인은 00_i2c_scanner로!

  연결: LCD 셀 → 디지털 '6'·'7' 칸 / 빛 셀 → Analog A2~A5 위치
  라이브러리: LiquidCrystal I2C
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED   2    // 내장 LED
#define LIGHT 34   // 빛센서 (A3 자리 예시 — 심화용, 없으면 관련 줄 삭제)
#define DARK  1000 // '가려짐' 기준값 — 시리얼로 관찰해서 교실에 맞게 조절

LiquidCrystal_I2C lcd(0x20, 16, 2);  // 고릴라셀 LCD 주소 = 0x20 (다른 제품이면 0x27·0x3F)

void setup() {
  pinMode(LED, OUTPUT);
  Wire.begin(27, 14);  // SDA='6'칸(27), SCL='7'칸(14) — I2C 블록(21·22)에 꽂았다면 21, 22로
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
