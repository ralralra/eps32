# 5단계 — 스마트 보관함 (서보 잠금 + 리드스위치 + 구글 시트 DB)

슬롯 4칸 보관함입니다. 슬롯마다 **서보(잠금)와 리드스위치(우산 감지)가 짝**을 이룹니다.
리드스위치가 감지되면(자석 가까이) 우산 있음, 감지 안 되면 우산 없음.

## 동작 규칙

**대여** — 앱(또는 시리얼)에서 대여 버튼:
```
슬롯 열림(100도) → 5초 → 닫힘(20도) → 리드 확인
  우산 없음  → ✓ 대여완료
  우산 그대로 → ✗ 대여실패 — 시트의 대여 기록 자동 취소(cancel)
```

**반납** — 해당 우산의 반납 버튼:
```
슬롯 열림 → 5초 → 닫힘 → 리드 확인
  우산 감지   → ✓ 반납완료
  감지 안 됨  → 다시 열렸다가 닫힘 (최대 3회 반복, 그래도 없으면 반납실패)
```

각도(20/100), 열림 시간(5초), 반복 횟수(3회)는 코드 위쪽 상수만 바꾸면 됩니다.

전체 실행 로직 순서도 → **[`flowchart.md`](flowchart.md)**

## 명령 넣는 두 가지 방법

| 방법 | 대여 | 반납 | 상태 보기 |
|---|---|---|---|
| 앱 → Apps Script 명령 큐 (3초 폴링) | `RENT:2` | `RETURN:2` | — |
| **시리얼 모니터 (115200)** | `r1`~`r4` | `b1`~`b4` | `s` |

WiFi가 안 잡혀도 15초 뒤 **시리얼 전용 모드**로 켜지므로, 앱 없이 배선·기구 테스트가 가능합니다.

## 준비물

- Wemos D1 R32 + **아두이노 센서쉴드** + USB
- **서보모터(SG90) ×4** — 슬롯 잠금
- **리드스위치 ×4** + 자석 — 우산 거치 감지
- 외부 5V 전원 (실드 EXT PWR 단자에 연결) — 서보용
- 구글 시트 DB (users / lockers / umbrellas / rentals / plans 탭)

배선 → **[`../docs/wiring_servo_reed.md`](../docs/wiring_servo_reed.md)**
(센서쉴드 기준: 서보 = 실드 5·4·3·2 자리, 리드 = 9·7·6·A1 자리)

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

### ② Apps Script 배포
1. 시트에서 `확장 프로그램 → Apps Script` → [`apps_script/umbrella_locker.gs`](apps_script/umbrella_locker.gs) 붙여넣기
2. 탭 이름이 다르면 코드 맨 위 `TAB` 설정만 수정
3. `배포 → 새 배포 → 웹 앱` / **액세스 권한: 모든 사용자** → **/exec URL 복사**

### ③ 시리얼로 먼저 테스트 (WiFi·앱 없이!)
업로드 후 시리얼 모니터(115200)에서:
```
s     → 슬롯 4칸의 우산 유무가 맞게 나오는지 (자석을 대보며 확인)
r1    → 슬롯 1 열림 → 5초 → 닫힘 → 우산 뺐으면 "대여완료"
b1    → 슬롯 1 열림 → 5초 → 닫힘 → 우산 꽂았으면 "반납완료",
        안 꽂으면 자동으로 다시 열림 (3회까지)
```

### ④ 앱 연결
[`step5_locker.ino`](step5_locker.ino)에서 WiFi 이름·비번과 **/exec URL** 수정 후 업로드.
라이브러리: **ESP32Servo** (기본 Servo.h는 ESP32에서 안 됩니다!)

## API 정리 (Apps Script)

| 요청 | 누가 | 하는 일 |
|---|---|---|
| `?action=rent&user_id=U001` | 앱 | 빈 우산 배정 → rentals 추가 + umbrellas·lockers 갱신 + `RENT:슬롯` 명령 |
| `?action=return&user_id=U001` | 앱 | 반납 처리 (이용 시간 계산) + `RETURN:슬롯` 명령 |
| `?action=status` | 앱 | 우산 현황 JSON |
| `?action=cmd` | ESP32 | 명령 읽기 (읽으면 큐가 비워짐) |
| `?action=report&p1=1&p2=0…` | ESP32 | 슬롯별 우산 유무 보고 → last_check_time 갱신 |
| `?action=cancel&slot=2` | ESP32 | **대여실패 롤백** — 우산이 안 빠졌을 때 rentals·umbrellas·lockers 원상복구 |

- 요금제: `&plan_id=P002` 추가 가능 (기본 P001=24시간)
- **정합성 규칙**: rentals·umbrellas·lockers는 항상 스크립트가 한 번에 갱신합니다.
  시트를 손으로 고치면 상태가 어긋나요! (LockService로 동시 요청도 방지)

## 알림 문법 (내장 LED)

| 상황 | LED |
|---|---|
| 대여완료·반납완료 | 짧게 2회 |
| 대여실패·반납실패 | 빠른 연속 점멸 |

## 응용 아이디어

- 웹앱(AI Studio)에 QR 스캔 붙이기 — `01_docs/ai_studio_webapp_guide.md`
- 연체 표시: `expected_return`이 지난 active 대여를 웹에서 빨갛게
- daily_stats 탭 집계 — 6회차 `03_stats_function.gs` 패턴 재활용
