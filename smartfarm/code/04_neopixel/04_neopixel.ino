/*
  2단계 ② 네오픽셀 RGB LED — 식물 생장등 & 상태등
  결선(키트 그대로): 확장쉴드 G(9)→GND · V(9)→VCC · S(9)→IN
  D1 R32 변환: D9 → GPIO13  ← 코드의 번호는 이것!
  라이브러리: Adafruit NeoPixel
  ⚠ 밝기를 너무 높이면 모듈이 뜨거워져요 — setBrightness 200 이하 권장
*/

#include <Adafruit_NeoPixel.h>

#define PIN    13   // D9 자리 (D1 R32에서 GPIO13)
#define NUMLED 4    // 모듈의 LED 개수 (키트 모듈에 맞게)

Adafruit_NeoPixel led(NUMLED, PIN, NEO_GRB + NEO_KHZ800);

void setColor(int r, int g, int b) {   // 모든 LED를 한 색으로
  for (int i = 0; i < NUMLED; i++) led.setPixelColor(i, led.Color(r, g, b));
  led.show();
}

void setup() {
  led.begin();
  led.setBrightness(150);
}

void loop() {
  setColor(255, 0, 0);     delay(1000);  // 빨강
  setColor(0, 255, 0);     delay(1000);  // 초록
  setColor(0, 0, 255);     delay(1000);  // 파랑
  setColor(255, 150, 60);  delay(2000);  // 따뜻한 빛 (생장등 느낌!)
  setColor(0, 0, 0);       delay(1000);  // 끄기
}
