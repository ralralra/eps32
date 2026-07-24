/*
  2단계 ① 쿨링 팬 — 2채널 모터 드라이버로 팬 2개 돌리기
  결선(키트 조립 그대로): 확장쉴드 S(4)→A-1B · S(5)→A-1A · S(6)→B-1A · S(7)→B-1B
                         G(4)→GND · V(4)→VCC / 팬은 모터 드라이버 MOTOR A·B 단자에
  D1 R32 변환: D4→17, D5→16, D6→27, D7→14  ← 코드의 번호는 이것!
  ⚠ 팬이 안 돌면 배터리(외부 전원)를 연결하세요 — USB 전원만으론 부족할 수 있어요
*/

#define AA 16  // D5 자리 → 팬A 방향1
#define AB 17  // D4 자리 → 팬A 방향2
#define BA 27  // D6 자리 → 팬B 방향1
#define BB 14  // D7 자리 → 팬B 방향2

void setup() {
  pinMode(AA, OUTPUT);
  pinMode(AB, OUTPUT);
  pinMode(BA, OUTPUT);
  pinMode(BB, OUTPUT);
}

void loop() {
  // 팬 두 개 켜기 (한쪽 HIGH + 반대쪽 LOW = 회전)
  digitalWrite(AA, HIGH); digitalWrite(AB, LOW);
  digitalWrite(BA, HIGH); digitalWrite(BB, LOW);
  delay(5000);   // 5초 동작

  // 팬 두 개 끄기
  digitalWrite(AA, LOW); digitalWrite(AB, LOW);
  digitalWrite(BA, LOW); digitalWrite(BB, LOW);
  delay(5000);   // 5초 정지
}

/*
  [응용 — 키트 자료의 속도 3단계 실습]
  digitalWrite 대신 analogWrite(AA, 100);  → 느리게
                    analogWrite(AA, 150);  → 중간
                    analogWrite(AA, 255);  → 최대
  (AB는 LOW 유지. 값 범위 0~255)
*/
