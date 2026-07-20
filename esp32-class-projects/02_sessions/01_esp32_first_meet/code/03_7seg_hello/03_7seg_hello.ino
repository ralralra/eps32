/*
  7세그먼트에 숫자 띄우기 — 보드가 숫자로 말해요!
  연결: images/wiring_7segment.png — 신호 2칸 포트에 (DIO='6'칸, CLK='7'칸)
  라이브러리: TM1637 (Avishay Orpaz) — 스케치 → 라이브러리 관리에서 설치
*/

#include <TM1637Display.h>

#define DIO 27  // '6' 칸
#define CLK 14  // '7' 칸

TM1637Display display(CLK, DIO);

void setup() {
  display.setBrightness(7);      // 밝기 0(어두움) ~ 7(최대)
  display.showNumberDec(1234);   // ★ 우리 팀 번호로 바꿔 보세요!
}

void loop() { }
