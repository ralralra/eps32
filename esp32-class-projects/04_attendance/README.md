# 스마트 출석 — 폰 웹 출석

학생이 **자기 폰으로 ESP32에 접속 → 이름 입력 → 출석 버튼** 을 누르면
출석이 기록되는 시스템. 
앱 설치도, 인터넷도, 추가 부품도 필요 없음

> 🧭 **보드 안내**: 이 프로젝트는 **Wemos D1 R32** 기준입니다. 추가 부품이 없어 보드만 바꾸면 그대로 동작해요.
> - 처음 세팅: [`01_docs/setup_wemos_d1_r32.md`](../01_docs/setup_wemos_d1_r32.md)
> - 고릴라셀 확장 아이디어: **웹 QR 체크인 + 홀센서로 문 열림 감지** → [6회차 — 출석팀 핵심 시나리오](../02_sessions/06_core_implementation/README.md)

## 무엇을 만드나

```
학생 폰  ──(와이파이)──▶  ESP32  ──▶  출석부 저장 (보드 안)
 브라우저                  (웹서버)        ▲
 이름+출석버튼                              │
                              선생님 폰으로 /list 접속해서 전체 확인·다운로드
```

- **부품**: Wemos D1 R32 1개. 끝. (센서·카드 없음)
- **인터넷**: 필요 없음. ESP32가 직접 와이파이를 만들어요(AP 모드)
- **앱**: 없음. 그냥 브라우저로 접속

## 왜 이걸 먼저?

- 가장 적은 부품으로 "센서 → 처리 → 저장 → 조회" 전체 흐름을 경험
- 웹페이지 / 버튼 / 저장 개념을 ESP32 없이 브라우저에서 먼저 익힘
- 여기서 배운 7단계 틀이 다른 모든 프로젝트에 그대로 쓰임

## 단계 안내

| 단계 | 폴더 | 무엇을 / 어디서 확인 |
|---|---|---|
| 1 | `step1_webpage` | 출석 웹페이지 만들기 → **PC 브라우저**로 확인 |
| 2 | `step2_list` | 출석 버튼 누르면 명단에 쌓이게 → **PC 브라우저** |
| 3 | `step3_save_browser` | 새로고침해도 안 사라지게 (저장 개념) → **PC 브라우저** |
| 4 | `step4_esp32_page` | 같은 페이지를 ESP32가 보여주기 → **폰으로 접속** |
| 5 | `step5_send_data` | 출석 버튼 데이터를 ESP32로 보내기 → **시리얼 모니터** |
| 6 | `step6_save_flash` | ESP32 안에 저장 (전원 꺼도 유지) → **폰 + 시리얼** |
| 7 | `step7_teacher` | 선생님용 전체 명단 + CSV 다운로드 → **폰/PC** |

각 폴더의 `README.md` 를 순서대로 따라가세요. 한 단계씩 **반드시 확인하고** 다음으로.

## 준비물 체크

- [ ] Wemos D1 R32 + USB 케이블
- [ ] Arduino IDE + esp32 보드 패키지 ([`docs/wiring_and_setup.md`](docs/wiring_and_setup.md))
- [ ] 크롬 같은 웹브라우저 (PC)
- [ ] 스마트폰 (와이파이 되는 것)

## 문서

- [`docs/wiring_and_setup.md`](docs/wiring_and_setup.md) — 보드 설정, 업로드 방법 (D1 R32는 쉬움!)
- [`docs/troubleshooting.md`](docs/troubleshooting.md) — 폰이 안 붙을 때, 페이지가 안 뜰 때 등

시작 → [`step1_webpage/README.md`](step1_webpage/README.md)
