# 5단계 — 스마트 보관함 (서보 잠금 + 리드센서 + 구글 시트 DB)

1~4단계의 "기록" 시스템을 **진짜 보관함**으로 업그레이드합니다.
폰에서 대여를 누르면 해당 슬롯의 잠금이 열리고, 리드센서가 우산이 실제로
빠졌는지/꽂혔는지 확인한 뒤 다시 잠급니다. DB는 **구글 시트**입니다.

```
[폰 웹앱] 대여/반납 ──▶ [Apps Script] 시트(DB) 갱신 + 명령 큐 "RENT:2"
                                          │
                        (3초마다 폴링)     ▼
[ESP32] 명령 수신 ──▶ 서보 열림 ──▶ 리드센서로 확인 ──▶ 서보 잠금 ──▶ 상태 보고
```

## 준비물

- Wemos D1 R32 + USB
- **서보모터(SG90) ×4** — 슬롯 잠금
- **리드센서(리드 스위치) ×4** + 자석 — 우산 거치 감지
- 외부 5V 전원 (건전지 팩 또는 5V 2A 어댑터) — 서보용
- 구글 시트 DB (users / lockers / umbrellas / rentals / plans 탭)

배선 → **[`../docs/wiring_servo_reed.md`](../docs/wiring_servo_reed.md)**

## 세팅 순서

### ① 구글 시트 준비
DB 시트에 아래 탭과 제목 행이 있어야 합니다 (열 **순서**는 달라도 OK — 코드가 제목으로 찾음):

| 탭 | 꼭 필요한 열 |
|---|---|
| users | user_id, name, status |
| lockers | locker_id, total_slots, available_slots, last_update |
| umbrellas | umbrella_id, locker_id, slot_no, status, last_check_time, total_rentals, last_user_id |
| rentals | rental_id, user_id, umbrella_id, locker_id, slot_no, plan_hours, plan_price, rental_time, expected_return, return_time, duration_min, status, payment_status |
| plans | plan_id, plan_name, hours, price, status |

- `commands` 탭은 **자동 생성**됩니다 (ESP32 명령 큐 — A1 셀)
- umbrellas의 status는 `available` / `rented` **영어로 통일**하세요
- ⚠ 형식이 안 맞는 행(슬롯 번호가 숫자가 아닌 행 등)은 대여 대상에서 제외됩니다

### ② Apps Script 배포
1. 시트에서 `확장 프로그램 → Apps Script` → [`apps_script/umbrella_locker.gs`](apps_script/umbrella_locker.gs) 내용 붙여넣기
2. 탭 이름이 다르면 코드 맨 위 `TAB` 설정만 수정
3. `배포 → 새 배포 → 웹 앱` / **액세스 권한: 모든 사용자** → **/exec URL 복사**

### ③ 브라우저로 먼저 테스트 (ESP32 없이!)
```
URL?action=status                 → 우산 현황 JSON이 보이면 연결 성공
URL?action=rent&user_id=U001      → rentals에 한 줄 + commands!A1에 "RENT:슬롯"
URL?action=cmd                    → "RENT:슬롯"이 보이고, 다시 열면 빈 값 (큐 소비)
URL?action=return&user_id=U001    → 반납 처리 확인
```

### ④ ESP32 업로드
[`step5_locker.ino`](step5_locker.ino)에서 WiFi 이름·비번과 **/exec URL** 수정 후 업로드.
라이브러리: **ESP32Servo** (기본 Servo.h는 ESP32에서 안 됩니다!)

## API 정리

| 요청 | 누가 | 하는 일 |
|---|---|---|
| `?action=rent&user_id=U001` | 웹앱 | 빈 우산 배정 → rentals 추가 + umbrellas·lockers 갱신 + 슬롯 열림 명령 |
| `?action=return&user_id=U001` | 웹앱 | 반납 처리 (이용 시간 계산) + 원래 슬롯 열림 명령 |
| `?action=status` | 웹앱 | 우산 현황 JSON |
| `?action=cmd` | ESP32 | 명령 읽기 (읽으면 큐가 비워짐) |
| `?action=report&p1=1&p2=0…` | ESP32 | 슬롯별 우산 유무 보고 → last_check_time 갱신 |

- 요금제: `&plan_id=P002` 추가 가능 (기본 P001=24시간)
- **정합성 규칙**: rentals·umbrellas·lockers 세 군데는 항상 스크립트가 한 번에
  갱신합니다. 시트를 손으로 고치면 상태가 어긋나요! (LockService로 동시 요청도 방지)

## 확인 방법 (핵심 시나리오)

1. 폰에서 [대여] → 3초 안에 해당 슬롯 서보가 열림
2. 우산을 빼면 → 리드센서가 감지 → 자동 잠금 + LED 짧게 2회
3. 시트 확인: rentals 새 줄(active) / umbrellas rented / lockers 슬롯 -1
4. [반납] → 같은 슬롯 열림 → 우산 꽂으면 잠금 → duration_min 계산됨

시간 초과(20초) 시에는 LED가 빠르게 점멸하고 슬롯을 다시 잠급니다.

## 응용 아이디어

- 웹앱(AI Studio)에 QR 스캔 붙이기 — `01_docs/ai_studio_webapp_guide.md`
- 연체 표시: `expected_return`이 지난 active 대여를 웹에서 빨갛게
- daily_stats 탭 집계 — 6회차 `03_stats_function.gs` 패턴 재활용
