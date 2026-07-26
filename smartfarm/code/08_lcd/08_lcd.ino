/*
  2단계 ③ LCD(I2C) — 스마트팜의 작은 상황판
  ⚠ 확장쉴드의 A4·A5 줄에 꽂으면 안 돼요! (D1 R32에서는 그 자리가 입력 전용)
     LCD의 SDA·SCL 선을 **보드 위쪽의 SDA·SCL 핀에 직접** 연결하세요.

  결선: LCD GND → 쉴드 G (아무 줄) / LCD VCC → 쉴드 V (아무 줄)
       LCD SDA → 보드 SDA 핀 (GPIO21) ← 직접!
       LCD SCL → 보드 SCL 핀 (GPIO22) ← 직접!
  라이브러리: LiquidCrystal I2C  ("avr 아키텍처" 경고는 무시해도 OK)
*/

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 키트 LCD 주소 = 0x27 (안 나오면 0x3F로)

int count = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Farm!");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print("count: ");
  lcd.print(count);
  count = count + 1;
  delay(1000);
}

/*
  [안 나올 때]
  1. 화면에 불은 켜지는데 글자가 없다 → 뒷면 파란 다이얼(대비)을 드라이버로 살짝 조절
  2. 주소 문제 → 0x27을 0x3F로 바꿔 재업로드
  3. 그래도 안 되면 → esp32-class-projects/08_cell_samples/00_i2c_scanner 로 실제 주소 확인
*/
