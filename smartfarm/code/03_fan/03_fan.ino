/*
  2단계 ① 쿨링 팬 — 2채널 모터 드라이버로 팬 2개 돌리기
  결선(키트 조립 그대로): 확장쉴드 S(4)→A-1B · S(5)→A-1A · S(6)→B-1A · S(7)→B-1B
                         G(4)→GND · V(4)→VCC / 팬은 모터 드라이버 MOTOR A·B 단자에
  D1 R32 변환: D4→17, D5→16, D6→27, D7→14  ← 코드의 번호는 이것!
  ⚠ 팬이 안 돌면 배터리(외부 전원)를 연결하세요 — USB 전원만으론 부족할 수 있어요
*/

// 모터 드라이버 제어 핀 4개 — 팬 하나에 핀 2개씩 (방향 결정용)
#define AA 16  // 팬A 방향1 (확장쉴드 D5 줄 = GPIO16)
#define AB 17  // 팬A 방향2 (확장쉴드 D4 줄 = GPIO17)
#define BA 27  // 팬B 방향1 (확장쉴드 D6 줄 = GPIO27)
#define BB 14  // 팬B 방향2 (확장쉴드 D7 줄 = GPIO14)

// 처음 한 번만 실행
void setup() {
  // 네 핀 모두 "출력용"으로 설정 — 보드가 이 핀으로 전기 신호를 내보낼 거예요
  pinMode(AA, OUTPUT);
  pinMode(AB, OUTPUT);
  pinMode(BA, OUTPUT);
  pinMode(BB, OUTPUT);
}

// 무한 반복
void loop() {
  // 팬 두 개 켜기 — 한쪽 핀은 HIGH(전기 O), 반대쪽 핀은 LOW(전기 X)여야 회전!
  // (두 핀이 같으면 멈춰요. HIGH/LOW를 서로 바꾸면 반대 방향으로 돌아요)
  digitalWrite(AA, HIGH); digitalWrite(AB, LOW);   // 팬A 회전
  digitalWrite(BA, HIGH); digitalWrite(BB, LOW);   // 팬B 회전
  delay(5000);   // 5초 동안 돌리기

  // 팬 두 개 끄기 — 모든 핀 LOW
  digitalWrite(AA, LOW); digitalWrite(AB, LOW);    // 팬A 정지
  digitalWrite(BA, LOW); digitalWrite(BB, LOW);    // 팬B 정지
  delay(5000);   // 5초 동안 정지
}

/*
  [응용 — 키트 자료의 속도 3단계 실습]
  digitalWrite 대신 analogWrite를 쓰면 속도 조절(PWM)이 돼요:
    analogWrite(AA, 100);  → 느리게
    analogWrite(AA, 150);  → 중간
    analogWrite(AA, 255);  → 최대
  (AB는 LOW 유지. 값 범위 0~255 — 값이 클수록 빨라요)
*/
