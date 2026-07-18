# 트러블슈팅

## 컴파일 (빌드) 단계 — 실전 사례

| 증상 | 원인 / 해결 |
|---|---|
| `undefined reference to 'Serial0'` / `'ESP'` / `'delay'` / `'pinMode'` 등 수십 줄 | **esp32 코어 3.x 캐시 깨짐**. (1) IDE 종료 (2) `%LOCALAPPDATA%\arduino\sketches\` 와 `\cores\` 폴더 통째 삭제 (3) 코어 **2.0.17** 로 다운그레이드 |
| `text section exceeds available space in board` | 파티션이 기본값. `Tools` → `Partition Scheme` → **Huge APP (3MB No OTA/1MB SPIFFS)** |
| `undefined reference to 'fd_forward'` 등 esp-face 심볼 | 코어 3.x 에서 CameraWebServer 의 얼굴 인식 빌드 시도 — 3.x 는 지원 안 함. 2.0.17 로 다운그레이드 |

## 업로드 단계 — 실전 사례

| 증상 | 원인 / 해결 |
|---|---|
| `Invalid head of packet (0x..): Possible serial noise or corruption` | Upload Speed 너무 높음 (460800/921600). **115200** 으로 낮춤. `Erase All Flash` 도 끔 |
| `Failed to connect to ESP32: Timed out waiting for packet header` | IO0 안 누르고 업로드 / GPIO0-GND 점프 안 됨 / RESET 타이밍 |
| `A fatal error occurred: MD5 of file does not match data in flash` | Upload Speed 너무 높음 → 115200으로 낮추기, USB 케이블 교체 |
| `Brownout detector was triggered` | 5V 전원 부족. USB 허브 X, 케이블 짧게, 외부 5V/2A 어댑터 |
| 포트(`/dev/ttyUSB0`, `COMx`)가 안 보임 | USB-UART 드라이버 미설치 (CH340/CP210x/FTDI) |
| 업로드 끝났는데 동작 안 함 (IO0 버튼식) | IO0 떼고 **RESET 짧게 한 번** 안 누름 |
| 업로드 끝났는데 동작 안 함 (FTDI) | GPIO0-GND 떼고 RESET 다시 |

## 부트 로그 해석

업로드 후 시리얼 모니터에서 본 첫 줄로 상태 파악:

| 보이는 메시지 | 의미 | 조치 |
|---|---|---|
| `boot:0x12 SPI_FAST_FLASH_BOOT` 다음에 정상 로그 | 일반 모드 부팅, 스케치 실행 중 | 그대로 진행 |
| `boot:0x12` 다음에 시리얼 침묵 | 스케치는 도는데 silent hang (카메라/Wi-Fi 코드 초반에서 멈춤) | CAMERA_MODEL 매크로, PSRAM, 카메라 리본 점검 |
| `boot:0x3 DOWNLOAD_BOOT` + `waiting for download` | 부트 모드(업로드 대기)에 갇힘 | IO0 떼고 RESET 짧게 한 번 |
| `rst:0xc (SW_CPU_RESET)` 반복, 같은 부트 로그가 1~2초 간격으로 다시 찍힘 | Brownout 리셋 루프 | 전원 부족 — 짧고 굵은 케이블, 외부 5V |
| 시리얼이 깨진 글자만 나옴 | 모니터 baud 불일치 | baud 115200 확인 |
| `Guru Meditation Error` / `Backtrace: ...` | 런타임 panic (NULL 참조, stack overflow 등) | 백트레이스 주소를 esp32 exception decoder 에 넣어 디버그 |

## 카메라 단계

| 증상 | 원인 / 해결 |
|---|---|
| `Camera init failed: 0x20004` | PSRAM Disabled. 보드설정에서 Enabled (2.0.17 메뉴) |
| `Camera init failed: 0x105` (ESP_ERR_NOT_FOUND) | OV2640 인식 실패. 모듈 리본 케이블 다시 끼우기, 전원 부족 의심 |
| `Camera init failed: 0x103` | 핀 충돌. **`CAMERA_MODEL_AI_THINKER` 매크로 활성화 안 됨**. 다른 `CAMERA_MODEL_xxx` 들도 전부 `//` 주석 확인 |
| 부팅 로그는 정상인데 카메라 init 메시지 없이 멈춤 | CAMERA_MODEL 매크로가 다른 보드로 돼 있어서 SCCB 통신 무한 대기 → 같은 원인 |
| 흰 화면/단색 화면 | XCLK 노이즈, 케이블 길이, 더 짧은 점퍼선 |
| 흐림/노출 이상 | `sensor_t::set_brightness/contrast/whitebal` 조정 |
| 초록 줄무늬 | 전원 노이즈. 디커플링 캡(100nF + 10uF) |

## Wi-Fi / 네트워크

| 증상 | 원인 / 해결 |
|---|---|
| 연결 안 됨 | 5GHz 전용 SSID 아닌지 확인 (ESP32는 2.4GHz만) |
| 자주 끊김 | RSSI -75dBm 이하. 안테나 방향, 거리 |
| 외부 안테나 모델인데 신호 약함 | 보드의 0Ω 점퍼(또는 회로 패턴)가 PCB 안테나로 가 있을 수 있음 — 외부 안테나 쪽으로 옮겨야 함 |

## 전력

- 카메라 켜진 ESP32-CAM 피크 전류는 ~600mA. PC USB(500mA)로 빠듯함.
- 가능하면 외부 5V/2A 또는 ESP32-CAM-MB 어댑터 + 좋은 케이블.

## 디버깅 팁

- 시리얼 모니터 baud 115200 고정 (코드와 일치)
- `ESP.getFreeHeap()` / `ESP.getPsramSize()` 출력으로 메모리 상태 늘 확인
- 카메라 init 실패 시 보드 보호회로가 발열로 자동차단된 적이 있는지 — 잠시 빼서 식히고 재시도

## 황금 진단 순서 (꽉 막혔을 때 위에서부터 점검)

1. **esp32 코어 버전**: 2.0.17 인가? 3.x 면 다운그레이드 + 캐시 삭제
2. **보드 선택**: `AI Thinker ESP32-CAM` 인가? (ESP32 Dev Module 아님)
3. **Tools 옵션**: Huge APP / PSRAM Enabled / Upload Speed 115200
4. **CAMERA_MODEL_AI_THINKER** 만 `//` 없이 활성화돼 있는가
5. **IO0 누른 채 Upload → 떼고 RESET** 절차 지켰는가
6. **시리얼 모니터 baud 115200** 맞는가, 포트 맞는가
7. **카메라 리본 케이블** 끝까지 들어갔는가
8. **전원**: PC USB 직결, 짧고 굵은 케이블
