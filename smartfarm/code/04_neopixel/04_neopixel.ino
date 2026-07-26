/*
  2단계 ② 네오픽셀 RGB LED — 식물 생장등 & 상태등
  결선(키트 그대로): 확장쉴드 G(9)→GND · V(9)→VCC · S(9)→IN
  D1 R32 변환: D9 → GPIO13  ← 코드의 번호는 이것!
  라이브러리: Adafruit NeoPixel
  ⚠ 밝기를 너무 높이면 모듈이 뜨거워져요 — setBrightness 200 이하 권장
*/

#include <Adafruit_NeoPixel.h>   // 네오픽셀 LED 제어용 라이브러리 불러오기

#define PIN    13   // 네오픽셀이 연결된 핀 (확장쉴드 D9 줄 = GPIO13)
#define NUMLED 12   // 모듈에 달린 LED 알갱이 개수 (우리 모듈은 12개)

// 네오픽셀 객체 생성 (LED 개수, 핀 번호, 색 순서와 통신 속도 설정)
Adafruit_NeoPixel led(NUMLED, PIN, NEO_GRB + NEO_KHZ800);

// 모든 LED를 한 가지 색으로 바꾸는 함수 (r, g, b = 빨강·초록·파랑의 세기, 각 0~255)
void setColor(int r, int g, int b) {
  for (int i = 0; i < NUMLED; i++)                    // 12개의 LED를 하나씩 돌면서
    led.setPixelColor(i, led.Color(r, g, b));         // i번째 LED에 색 지정 (아직 화면엔 반영 안 됨)
  led.show();                                         // 지정한 색을 실제 LED에 반영!
}

// 처음 한 번만 실행
void setup() {
  led.begin();              // 네오픽셀 작동 시작
  led.setBrightness(150);   // 전체 밝기 설정 (0~255 — 너무 높으면 뜨거워요)
}

// 무한 반복 — 색이 1초씩 바뀌어요
void loop() {
  setColor(255, 0, 0);     delay(1000);  // 빨강 (R만 최대)
  setColor(0, 255, 0);     delay(1000);  // 초록 (G만 최대)
  setColor(0, 0, 255);     delay(1000);  // 파랑 (B만 최대)
  setColor(255, 150, 60);  delay(2000);  // 따뜻한 빛 (빨강+초록 조금+파랑 조금 = 주황빛)
  setColor(255, 0, 255);   delay(2000);  // 보라색 (빨강+파랑 = 실제 식물 생장등 색!)
  setColor(0, 0, 0);       delay(1000);  // 끄기 (모든 색 0)
}
