# 보드 설정 & 업로드 방법 (Wemos D1 R32)

> 처음부터 자세한 세팅(드라이버·IDE·보드 패키지)은
> **[공통 세팅 가이드 → `01_docs/setup_wemos_d1_r32.md`](../../01_docs/setup_wemos_d1_r32.md)** 를 먼저 보세요.
> 핀맵은 [`01_docs/pinmap_wemos_d1_r32.md`](../../01_docs/pinmap_wemos_d1_r32.md) 참고.

## 회로 — 없음!

이 프로젝트는 **Wemos D1 R32 보드 1개와 USB 케이블** 만 있으면 됩니다.
센서도, 카드 리더도, 점퍼선도 필요 없어요. 보드를 USB로 PC에 꽂기만 하면 끝.

## Wemos D1 R32 는 어떤 보드?

| | 아두이노 우노 | Wemos D1 R32 (이 프로젝트) |
|---|---|---|
| 생김새 | 우노 모양 | **우노와 똑같은 모양** (고릴라 실드 호환) |
| WiFi | 없음 | **ESP32 내장 — WiFi 됨!** |
| USB 칩 | — | **CH340** → 드라이버 필요 |
| 업로드 | — | **그냥 Upload 클릭** (대부분 자동) |

→ 포트(COM)가 안 보이면 십중팔구 **CH340 드라이버** 문제입니다.
설치 방법: [공통 세팅 가이드 1번 항목](../../01_docs/setup_wemos_d1_r32.md#1-ch340-드라이버-설치-최초-1회)

## Arduino IDE 설정

1. esp32 보드 패키지 설치 — [공통 세팅 가이드](../../01_docs/setup_wemos_d1_r32.md) 참고
2. `Tools` → `Board` → `ESP32 Arduino` → **ESP32 Dev Module** (목록에 WEMOS D1 R32가 있으면 그것도 OK)
3. `Tools` → `Port` → 보드가 잡힌 COM/tty 선택 (뽑았다 꽂아서 새로 생긴 번호!)
4. `Tools` → `Upload Speed` → **115200** (안정적)

> SPIFFS(보드 내부 저장)를 쓰는 6·7단계에서는 파티션에 SPIFFS 공간이 있어야 합니다.
> `Tools` → `Partition Scheme` → **Default 4MB with spiffs** (기본값이면 보통 OK)

## 업로드 방법

1. 코드 열기 → 상단 **Upload (→)** 버튼 클릭
2. `Writing...` 진행되면 정상 (D1 R32는 대부분 자동 부팅)
3. 만약 `Connecting....._____` 가 길게 나오면 보드의 **BOOT(IO0) 버튼**을 잠깐 누르고 있기
4. `Hash of data verified` → `Hard resetting` 나오면 성공

## 시리얼 모니터

- `Tools` → `Serial Monitor` → 우측 하단 **115200 baud**
- 코드가 `Serial.begin(115200)` 이라 일치시켜야 글자가 안 깨짐

## AP 모드란? (이 프로젝트의 핵심)

ESP32가 **스스로 와이파이를 만들어** 학생 폰이 거기 접속합니다.
공유기/인터넷이 필요 없어요.

```
D1 R32  ──(와이파이 'ESP32-Attend' 송출)──  학생 폰이 이 와이파이에 접속
         접속 후 브라우저로 http://192.168.4.1
```

- 학생 폰은 이 와이파이에 붙는 동안 **인터넷이 안 됨** (정상)
- 폰이 "인터넷 없음, 계속 연결할까요?" 물으면 **"계속 연결"** 선택
- AP 모드 원리 수업: [3회차 — ESP32 웹서버](../../02_sessions/03_esp32_webserver/README.md)
