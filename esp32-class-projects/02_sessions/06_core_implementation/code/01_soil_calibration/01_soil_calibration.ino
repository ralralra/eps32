/*
  6회차 · 화분팀 특별 미션 — 토양습도 셀 캘리브레이션
  3대 규칙: 포트 위치(A2~A5) · 기준값 측정 · 해석

  진행:
  1단계 — 원시값 관찰: 공기 중(건조) 값과 물컵 속(습윤) 값을 시리얼로 기록
  2단계 — 아래 DRY/WET 에 기록한 값을 넣으면 0~100%로 번역!

  다른 팀도 동일 원리:
    우산팀 = 초음파·무게 기준값(우산 있음/없음)
    출석팀 = 홀센서·자석 감지 거리
    매점팀 = 무게 영점과 상품 1개 무게
*/

#define SOIL 34  // A2~A5 위치 포트 (GPIO 34·35·36·39 — 핀맵 카드 확인)
// 왜 이 자리? WiFi 사용 중엔 다른 아날로그 자리(ADC2)가 먹통이기 때문!

// ★ 1단계에서 측정한 우리 팀 기준값으로 교체
int DRY = 3100;  // 공기 중(건조) 값
int WET = 1300;  // 물컵 속(습윤) 값
// ⚠ %가 반대로 움직이면(젖을수록 작아짐) DRY·WET 순서를 서로 바꾸면 돼요

void setup() {
  Serial.begin(115200);
}

void loop() {
  // 1단계: 원시값 관찰
  int raw = analogRead(SOIL);

  // 2단계: 0~100%로 번역
  int pct = map(raw, DRY, WET, 0, 100);
  pct = constrain(pct, 0, 100);  // 0~100 범위 밖은 잘라내기

  Serial.print("raw: ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(pct);
  Serial.println("%");
  delay(500);
}
