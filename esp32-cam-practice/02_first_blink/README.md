# 2단계 — 첫 업로드 (Blink + Hello)

![ ](../images/image%20(1%29.png)

ESP32-CAM에는 보드 뒷면에 **빨간 LED(GPIO33, active-low)** 와 앞면 **흰색 플래시 LED(GPIO4)** 가 있습니다.

목표: 시리얼에 메시지 출력하면서 빨간 LED 깜빡여 보는 것.
이 단계가 성공하면 USB-UART, 부트 모드 전환, 시리얼 통신 셋업이 모두 정상이라는 뜻.

> ⚠ 시작 전 확인 — esp32 코어 **2.0.17** 깔려 있는지. 3.x 면 1단계 `README.md` 의 다운그레이드 절차 먼저 진행.

## 회로
- 추가 부품 없음. USB-UART / 어댑터만 연결.

## 코드
`blink.ino` 참고.

## 실행 방법 (ESP32-CAM-MB 어댑터 또는 IO0 버튼 보드)

1. Arduino IDE에서 `blink.ino` 열기
2. 보드 설정 1단계대로 (AI Thinker ESP32-CAM, Huge APP, PSRAM Enabled, Upload Speed 115200)
3. SSID/PW 설정 불필요 (이 단계는 Wi-Fi 안 씀)
4. 업로드 사이클:
   - **IO0 버튼 누른 채로** 유지
   - Arduino IDE → **Upload** 클릭
   - "Connecting....." 점 찍히는 동안 IO0 계속 누름
   - `Writing... 100%` 또는 `Hash of data verified` 나오면 IO0 떼기
   - **RESET 버튼 짧게 한 번** 누르기
5. `Tools` → `Serial Monitor` → **115200 baud** (우측 하단)
6. 결과 확인:
   - 보드 **뒷면 빨간(약간 보라) LED 가 1초 간격으로 점멸**
   - 시리얼에 `tick 1, tick 2, ...` 카운트 출력
   - 첫 줄에 칩 정보 (`ESP32-D0WD-V3 rev ..., 2 cores @ 240 MHz`)와 `Flash: 4 MB, PSRAM: 4194304 bytes`

> 흰색 플래시 LED(GPIO4)는 코드에서 **일부러 꺼둠** (눈에 너무 시려서). 점멸 안 되는 게 정상.

## 실행 방법 (FTDI 직결)

1~3은 위와 동일. 4번 업로드 사이클만:
- GPIO0 ↔ GND 점퍼 연결 → RESET 한 번 → Upload 클릭 → 끝나면 GPIO0 떼고 RESET

## 잘 안 될 때 1분 진단

| 시리얼에 보이는 것 | 해석 | 조치 |
|---|---|---|
| 아무것도 안 나옴, 빨간 LED 깜빡임 | 보드는 도는데 모니터 baud/포트 다름 | 모니터 baud 115200, 포트 재선택 |
| 아무것도 안 나옴, 빨간 LED도 X | 부트 모드에 갇혀 있음 | IO0 떼고 RESET만 짧게 |
| `boot:0x3 DOWNLOAD_BOOT` `waiting for download` | IO0 누른 채 RESET 함 | IO0 떼고 RESET만 다시 |
| `boot:0x12 SPI_FAST_FLASH_BOOT` 만 나오고 멈춤 | 스케치는 도는데 silent hang | 코드/라이브러리 문제 (blink.ino면 거의 안 일어남) |
| `rst:0xc (SW_CPU_RESET)` 반복 | Brownout 리셋 루프 | 전원 부족 — 케이블/외부 5V |
| `Hash of data verified` 후 아무 것도 X | RESET 안 누름 | RESET 짧게 한 번 |

## 배우는 것
- `Serial.begin()` 으로 디버깅 출력
- `pinMode()` / `digitalWrite()` 기본
- ESP32-CAM의 GPIO33은 **반전 로직** (LOW = 켜짐)
- IO0 누른 채 업로드 → 떼고 RESET 으로 일반 모드 전환 (몸으로 익히기)
