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
박람회 출품 기준 현실성 점검·대체안 → **[`EXPO_PLAN.md`](EXPO_PLAN.md)**

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
| plans | plan_id, plan_name, hours, price, extra_24h_price, status |

- `commands` 탭은 **자동 생성**됩니다 (ESP32 명령 대기줄 — 위에서부터 차례로 실행)
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
[`step5_locker.ino`](step5_locker.ino)에서 WiFi 이름·비번, **/exec URL**, 그리고
**`LOCKER_ID`**(이 보드가 담당할 보관함 — 시트 lockers 탭의 값, 기본 `L001`)를 확인 후 업로드.
라이브러리: **ESP32Servo** (기본 Servo.h는 ESP32에서 안 됩니다!)

## 앱 연동 (화면별 코드)

기획안 10개 화면에서 무엇을 호출하고 무엇이 돌아오는지 → **[`app/screens.md`](app/screens.md)**
공통 호출 코드는 [`app/api.js`](app/api.js) (`SERVER_URL` 한 줄만 수정).
AI Studio 지시문 → 처음부터 만들 때 **[`app/build_prompt.md`](app/build_prompt.md)** · 이미 만든 앱 고칠 때 [`app/ai_studio_prompt.md`](app/ai_studio_prompt.md)

## API 정리 (Apps Script)

| 요청 | 누가 | 하는 일 |
|---|---|---|
| `?action=register&name=..&phone=..` | 앱 | 회원 등록 (같은 번호면 기존 user_id 반환) |
| `?action=register_account&user_id=..&account=..` | 앱 | 결제 계좌 등록 (마스킹 저장) |
| `?action=lockers` | 앱 | 보관함 목록 |
| `?action=rent&user_id=U001&locker_id=L001&umbrella_id=UB002` | 앱 | 지정 우산 대여 → rentals 추가 + umbrellas·lockers 갱신 + `RENT:슬롯` 명령 |
| `?action=return&user_id=U001` | 앱 | 반납 + **후불 정산** (이용시간·초과요금 계산 → payments 기록) + `RETURN:슬롯` |
| `?action=history&user_id=U001` | 앱 | 이용·결제 내역 |
| `?action=status&locker_id=L001` | 앱 | 우산 현황 JSON |
| `?action=cmd` | ESP32 | 명령 읽기 (읽으면 큐가 비워짐) |
| `?action=report&locker_id=L001&p1=1&p2=0…` | ESP32 | 슬롯별 우산 유무 보고 → last_check_time 갱신 |
| `?action=plans` | 앱 | 요금제 목록 (시트 plans 탭) |
| `?action=cancel&locker_id=L001&slot=2` | ESP32 | **대여실패 롤백** — 우산이 안 빠졌을 때 rentals·umbrellas·lockers 원상복구 |

- 요금제: `&plan_id=P002` 추가 가능 (기본 P001=24시간)
- **정합성 규칙**: rentals·umbrellas·lockers는 항상 스크립트가 한 번에 갱신합니다.
  시트를 손으로 고치면 상태가 어긋나요! (LockService로 동시 요청도 방지)

## 문제 해결 — 앱은 되는데 모터가 안 움직일 때

**먼저 시트의 `commands` 탭을 보세요.** 여기서 원인이 갈립니다.

| commands 탭 상태 | 뜻 | 할 일 |
|---|---|---|
| 명령이 쌓인 채 안 없어짐 | 앱·서버는 정상, **보드가 서버에 연결 안 됨** | 아래 ①②③ |
| 명령이 바로 사라짐 | 보드가 받아감 → **모터·전원 문제** | 아래 ④ |
| 애초에 안 쌓임 | **앱이 서버를 못 부름** | 브라우저 콘솔에서 CORS 확인 |

보드는 3초마다 명령을 가져갑니다. 명령이 1분 넘게 남아 있으면 보드가 못 오고 있는 겁니다.

### ① 시리얼 모니터로 보드 상태 확인 (제일 먼저!)
USB를 연결하고 시리얼 모니터를 **115200**으로 연 뒤 보드의 EN(리셋) 버튼을 누르세요.

| 나오는 메시지 | 뜻 |
|---|---|
| `WiFi 연결 완료 — 앱 명령 대기!` | 정상 — ④로 |
| `WiFi 없음 — 시리얼 전용 모드` | WiFi 실패 → ② |
| 아무것도 안 나옴 | 업로드가 안 됐거나 보드 전원 문제 |

### ② WiFi가 안 잡힐 때
- **ESP32는 5GHz WiFi에 연결할 수 없어요.** 반드시 **2.4GHz**를 쓰세요.
  폰 핫스팟이면 설정에서 대역을 2.4GHz로 바꿔야 합니다. (가장 흔한 원인!)
- 학교 WiFi처럼 로그인 페이지가 뜨는 곳은 연결이 안 됩니다 → 폰 핫스팟 권장
- 코드의 `WIFI_SSID` / `WIFI_PASS` 오타 확인

### ③ 주소·보관함 번호 확인
- `URL`이 아직 `.../XXXX/exec` 그대로면 서버에 못 갑니다 → 시트 `시트1` 탭의 주소로 교체
- `LOCKER_ID`가 시트 `lockers` 탭의 값과 같은지 (기본 `L001`)

### ④ 모터가 도는지만 따로 확인
시리얼 모니터에 `b3` 입력 → 3번 칸이 열렸다 닫히면 모터·배선은 정상입니다.

**모터가 이상하게 움직일 때는 증상으로 원인을 가릅니다** — 시리얼을 보면서 돌려보세요.

| 증상 | 원인 | 해결 |
|---|---|---|
| 움직이는 순간 시리얼에 **부팅 메시지가 다시 뜸** | 전원 부족 (보드가 리셋됨) | 외부 5V 2A 이상 |
| 부르르 떨리기만 하고 힘이 없음 | 전원 부족 또는 공통 접지 누락 | 아래 확인 |
| 하나씩은 잘 도는데 여러 개 쓰면 이상 | 전원 용량 부족 | 외부 5V 2A 이상 |
| 끝까지 안 가고 걸린 채 웅웅거림 | **기구 간섭** (전원 아님) | 걸쇠 위치·각도 조정 |
| 아예 무반응 | 배선 (신호선 자리 틀림) | 실드 5·4·3·2 자리 확인 |

**전원 기준**: SG90 하나가 움직이는 순간 최대 **0.7A** 정도를 먹습니다.
USB(5V 0.5A)로는 한 개도 빠듯해요. **5V 2A 이상 어댑터**나 건전지팩을 쓰세요.

> ❌ **12V 어댑터를 보드 DC잭에 꽂지 마세요.** 그 12V가 5V 핀·실드 V줄로 그대로 나와
> 서보(4.8~6V)를 태울 수 있습니다. 12V밖에 없다면 벅 컨버터(LM2596)로 5V를 만들어 쓰세요.
> 자세한 이유와 확인 방법 → [`../docs/wiring_servo_reed.md`](../docs/wiring_servo_reed.md)

- 서보 전원을 **외부 5V**에서 받고 있는지 (보드 전원으로는 못 돌립니다)
- 외부 전원의 **GND와 보드 GND가 연결**돼 있는지
  (공통 접지 — 빠지면 서보가 신호를 못 알아들어 떨리기만 합니다)

> 💡 코드에서 `RELEASE_WHEN_IDLE = true`면 **대기 중에는 서보 힘을 빼둡니다.**
> 전류·떨림·발열이 크게 줍니다. 문을 손으로 못 열게 계속 버텨야 한다면 `false`로 바꾸세요
> (대신 전원이 넉넉해야 합니다).

### ⑤ 밀린 명령 정리
보드가 꺼져 있는 동안 누른 명령이 큐에 그대로 쌓입니다.
연결되는 순간 **오래된 것부터 실행**되므로, 엉뚱한 칸이 열릴 수 있어요.
→ 시트 `commands` 탭에서 **헤더(1행)만 남기고 나머지 줄을 지우세요.**

## 알림 문법 (내장 LED)

| 상황 | LED |
|---|---|
| 대여완료·반납완료 | 짧게 2회 |
| 대여실패·반납실패 | 빠른 연속 점멸 |

## 응용 아이디어

- 웹앱(AI Studio)에 QR 스캔 붙이기 — `01_docs/ai_studio_webapp_guide.md`
- 연체 표시: `expected_return`이 지난 active 대여를 웹에서 빨갛게
- daily_stats 탭 집계 — 6회차 `03_stats_function.gs` 패턴 재활용
