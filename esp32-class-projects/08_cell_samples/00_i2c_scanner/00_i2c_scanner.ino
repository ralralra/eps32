/*
  I2C 스캐너 — I2C 장치가 안 잡힐 때 '진짜 주소'를 찾아주는 탐정 코드
  I2C 셀을 꽂은 채 업로드 → 시리얼 모니터(115200) 확인!

  결과 읽는 법:
    "발견! 주소 = 0x20"  → 코드의 lcd(0x20, 16, 2) 그대로 OK
    "발견! 주소 = 0x27"  → lcd(0x27, 16, 2) 로 바꾸기
    "아무것도 못 찾음"    → 주소 문제가 아니라 배선·포트·전원 문제!
                           (케이블 순서, 4번째 포트 SDA·SCL 반전, 실드 전원 스위치 확인)

  ⚠ 실드 I2C 블록(기본 21·22)에서 계속 안 잡히면 — 이 실드가 우노 A4·A5 라인 기준이라
    D1 R32에선 입력 전용 핀(36·39)으로 이어져 I2C가 불가능한 경우예요.
    셀을 디지털 '6'·'7' 칸에 꽂고 아래를 Wire.begin(27, 14); 로 바꿔 다시 스캔!
*/

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();          // 기본: SDA=21, SCL=22 / '6'·'7' 칸에 꽂았다면 Wire.begin(27, 14);
  delay(1000);
  Serial.println("\nI2C 스캔 시작...");
}

void loop() {
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {   // 응답이 오면 장치 존재!
      Serial.printf("발견! 주소 = 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0)
    Serial.println("아무것도 못 찾음 — 배선·포트·전원을 확인하세요");
  Serial.println("--- 3초 뒤 다시 스캔 ---");
  delay(3000);
}
