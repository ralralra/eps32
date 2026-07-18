# 🐣 왕초보 가이드 — 모든 실습을 순서대로, 클릭 단위로

> 코딩·전자가 처음이어도 이 문서만 따라가면 됩니다.
> 모든 실습이 같은 구조예요: **🔌 배선 그림 → 📄 코드 → 👣 순서 → ✅ 성공 확인 → 🚨 안 될 때**

## 시작 전 딱 한 번 — 컴퓨터 준비

아직 안 했다면 [보드 세팅 가이드](../01_docs/setup_wemos_d1_r32.md)를 먼저 하세요. 요약:

1. **CH340 드라이버** 설치 (없으면 보드가 컴퓨터에 안 잡혀요 — 실패 원인 1위)
2. **Arduino IDE** 설치
3. IDE에 **esp32 보드 지원** 설치 (파일→환경설정→URL 붙여넣기→보드 매니저에서 esp32 설치)
4. 보드 연결 → `툴→보드→ESP32 Dev Module`, `툴→포트→COM○` 선택
5. **라이브러리 3개** 미리 설치: `스케치→라이브러리 포함→라이브러리 관리`에서
   `LiquidCrystal I2C` / `DHT sensor library` / `TM1637` 검색해서 각각 [설치]

## 회차별 가이드

| 문서 | 내용 |
|---|---|
| [1회차 — LED·LCD·첫 센서](guide_session1.md) | 첫 업로드부터 카운트다운 타이머까지 |
| [2회차 — 센서들 + 첫 웹페이지](guide_session2.md) | 시리얼 관찰, 경보기, HTML·CSS·JS |
| [3회차 — 폰으로 보드 조종](guide_session3.md) | ESP32 WiFi 만들기, 진짜 리모컨 |
| [4회차 — 구글 시트에 저장](guide_session4.md) | Apps Script 배포부터 원격 LED까지 |

## 꼭 기억할 5가지 (모든 실습 공통)

1. **코드는 통째로 복사** — IDE에서 `파일→새 스케치` → 전체 지우고 → 붙여넣기. 한 글자도 직접 안 쳐도 돼요
2. `//` 뒤의 글은 **설명 메모(주석)** — 지워도 되고 그대로 둬도 됩니다. 뭘 채워 넣는 곳이 아니에요!
3. **아날로그 셀은 A2~A5 위치** 포트에만 (그림마다 표시해 뒀어요)
4. 코드의 핀 번호는 실드에 인쇄된 숫자가 아니라 **변환된 GPIO** — 각 그림의 노란 칸 숫자를 쓰세요
5. 업로드 버튼은 **화살표(→)** — 체크(✓)는 검사만 하고 보드에 안 들어가요

## 그림 모음 (인쇄해서 나눠주기 좋아요)

- [실드 전체 GPIO 핀맵](../images/gorilla_shield_x_d1r32_pinmap.png) — 모든 포트의 번호표
- [LCD 연결](../images/lcd_wiring_gorilla_shield.png) · [아날로그 셀](../images/wiring_analog_cell.png) · [디지털 셀](../images/wiring_digital_cell.png) · [초음파](../images/wiring_ultrasonic.png) · [7세그먼트](../images/wiring_7segment.png)
- [환경 모니터 셀 3개 동시 연결](../images/wiring_env_monitor.png) · [구글 시트 탭 구성](../images/sheet_tabs_guide.png)
