// 시뮬레이터용 ESP32Servo 흉내 — 각도와 attach 상태를 기록만 합니다.
#pragma once
#include "Arduino.h"

namespace sim {
  struct ServoState { bool attached = false; int angle = -1; int pin = -1; int moves = 0; };
  extern ServoState servoState[40];      // 핀 번호로 찾습니다
  extern bool allocatedTimer[4];
}

class ESP32PWM {
 public:
  static void allocateTimer(int n) { if (n >= 0 && n < 4) sim::allocatedTimer[n] = true; }
};

class Servo {
 public:
  int attach(int pin, int, int) {
    // 실제 라이브러리는 쓸 타이머가 없으면 0을 돌려주며 실패합니다.
    bool any = false;
    for (bool t : sim::allocatedTimer) any = any || t;
    if (!any) return 0;                  // allocateTimer를 안 부른 경우 재현
    pin_ = pin;
    sim::servoState[pin].attached = true;
    sim::servoState[pin].pin = pin;
    return 1;
  }
  bool attached() const { return pin_ >= 0 && sim::servoState[pin_].attached; }
  void detach() { if (pin_ >= 0) sim::servoState[pin_].attached = false; }
  void write(int angle) {
    if (pin_ < 0 || !sim::servoState[pin_].attached) return;   // 실패 시 아무 일도 없음
    if (sim::servoState[pin_].angle != angle) sim::servoState[pin_].moves++;
    sim::servoState[pin_].angle = angle;
  }
  void setPeriodHertz(int) {}
 private:
  int pin_ = -1;
};
