/*
  2단계 ③ LCD(I2C) — 스마트팜의 작은 상황판
  ⚠ 확장쉴드의 A4·A5 줄에 꽂으면 안 돼요! (D1 R32에서는 그 자리가 입력 전용)
     LCD의 SDA·SCL 선을 **보드 위쪽의 SDA·SCL 핀에 직접** 연결하세요.

  결선: LCD GND → 쉴드 G (아무 줄) / LCD VCC → 쉴드 V (아무 줄)
       LCD SDA → 보드 SDA 핀 (GPIO21) ← 직접!
       LCD SCL → 보드 SCL 핀 (GPIO22) ← 직접!
  라이브러리: LiquidCrystal I2C  ("avr 아키텍처" 경고는 무시해도 OK)
*/

#include <LiquidCrystal_I2C.h>   // I2C 방식 LCD 제어용 라이브러리 불러오기

// LCD 객체 생성 (I2C 주소, 가로 칸수, 세로 줄수)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 키트 LCD 주소 = 0x27 (안 나오면 0x3F로)

int count = 0;   // 숫자를 담아둘 변수 (상자) — 1초마다 1씩 커져요

// 처음 한 번만 실행
void setup() {
  lcd.init();                 // LCD 초기화 (통신 시작)
  lcd.backlight();            // 백라이트(화면 불빛) 켜기
  lcd.setCursor(0, 0);        // 커서를 첫째 줄(0), 첫째 칸(0)으로
  lcd.print("Smart Farm!");   // 글자 출력 (영문·숫자만 — 한글은 안 나와요!)
}

// 무한 반복
void loop() {
  lcd.setCursor(0, 1);        // 커서를 둘째 줄(1) 처음으로
  lcd.print("count: ");       // 글자 출력
  lcd.print(count);           // 변수에 담긴 숫자 출력
  count = count + 1;          // 숫자를 1 키우고
  delay(1000);                // 1초 쉬고 다시 (숫자가 실시간으로 변해요 = '갱신')
}

/*
  [안 나올 때]
  1. 화면에 불은 켜지는데 글자가 없다 → 뒷면 파란 다이얼(대비)을 드라이버로 살짝 조절
  2. 주소 문제 → 0x27을 0x3F로 바꿔 재업로드
  3. 그래도 안 되면 → esp32-class-projects/08_cell_samples/00_i2c_scanner 로 실제 주소 확인
*/
