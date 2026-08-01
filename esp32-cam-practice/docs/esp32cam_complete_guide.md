# ESP32-CAM 완전 정복 가이드 (정리본)

> [DIY Engineers — ESP32-Cam Complete Guide](https://www.diyengineers.com/2023/04/13/esp32-cam-complete-guide/) 내용을 한국어로 정리한 문서입니다.
> 원문 중 OpenCV 연동 부분은 제외하고, **보드 소개 / 핀맵 / 프로그래밍 세팅 / CameraWebServer 비디오 설정**을 중심으로 담았습니다.
> 이 저장소에서 실전 검증한 값과 다른 부분은 각주로 표시했습니다.

## 1. ESP32-CAM 이란?

ESP32-S 칩 위에 OV2640 카메라를 얹은 초소형(약 27×40mm) 카메라 모듈 보드.

| 항목 | 내용 |
|---|---|
| MCU | ESP32-S (듀얼코어 240MHz) |
| 카메라 | OV2640 (최대 1600×1200 UXGA) |
| Wi-Fi | 802.11 b/g/n (**2.4GHz 전용**) |
| Bluetooth | 4.2 + BLE |
| 플래시 LED | 앞면 흰색 고휘도 LED 내장 (GPIO 4) |
| IO 포트 | 사용 가능한 IO 핀 소수 (대부분 카메라/SD가 점유) |
| 통신 | UART, SPI, I2C, PWM 지원 |
| 저장 | micro SD 카드 슬롯 내장 |
| 입력 전원 | 3.3V / 5V — **5V 권장** |
| USB 포트 | **없음** — 크기를 줄이기 위해 생략됨. 업로드에 외장 USB-UART(FTDI 등) 필요 |

## 2. 핀맵 (AI Thinker 기준)

보드 바깥으로 나온 핀 기준 요약. (카메라 내부 배선까지 포함한 전체 GPIO 점유표는 [`pinmap.md`](pinmap.md) 참고)

| 핀 | 역할 | 비고 |
|---|---|---|
| 5V / 3.3V | 전원 입력 | 5V 권장. 3.3V는 전류 부족으로 brownout 잦음 |
| GND | 접지 | 2곳 |
| U0T (GPIO 1) | 시리얼 TX | FTDI **RX** 에 연결 |
| U0R (GPIO 3) | 시리얼 RX | FTDI **TX** 에 연결 |
| IO0 (GPIO 0) | **부트 모드 선택** | 업로드(플래싱) 중에는 반드시 GND에 연결 = LOW |
| IO2, IO4, IO12, IO13, IO14, IO15 | 범용 IO | 단, **SD 카드 사용 시 전부 점유됨** |
| IO16 | 범용 IO | PSRAM CS와 공유 — 사용 주의 |
| IO33 | 뒷면 빨간 LED | active-LOW |
| VCC / GND (카메라쪽) | — | — |

핵심 주의점:

- **GPIO 0** — 부팅 시 LOW면 "다운로드(업로드) 모드", HIGH(개방)면 일반 부팅. 업로드할 때만 GND에 점퍼로 묶고, 끝나면 반드시 떼고 리셋.
- **GPIO 4** — 흰색 플래시 LED와 공유. SD 카드(4-bit 모드) 사용 시 LED가 같이 켜지는 원인.
- **GPIO 2, 4, 12, 13, 14, 15** — micro SD 리더가 사용. SD를 안 쓰면 일반 IO로 활용 가능.
- **GPIO 1, 3** — 시리얼 업로드/디버깅 통로. 다른 용도로 쓰면 업로드가 안 됨.

## 3. FTDI 프로그래머 연결 (업로드 배선)

ESP32-CAM은 USB 포트가 없으므로 FTDI(또는 CP2102 등 USB-UART) 어댑터로 코드를 올립니다.

```
FTDI            ESP32-CAM
─────           ─────────
VCC (5V)  ───▶  5V
GND       ───▶  GND
TX        ───▶  U0R
RX        ───▶  U0T
                IO0 ──┐
                GND ──┘  (업로드하는 동안만 점퍼로 연결)
```

- FTDI의 전압 점퍼가 있다면 **5V** 쪽으로. (3.3V 공급 시 카메라 기동 전류를 못 버티는 경우 많음)
- TX/RX는 **교차**로 연결 (FTDI TX → 보드 U0R).
- **IO0–GND 점퍼는 "업로드하는 동안만"**. 플래싱 중 이 핀은 반드시 LOW여야 함.

### 업로드 절차

1. 위 배선 상태(IO0–GND 연결됨)로 FTDI를 PC USB에 연결
2. Arduino IDE에서 포트 선택 후 Upload
3. `Connecting...` 표시 중 보드의 **RESET 버튼**을 한 번 눌러 업로드 시작
4. `Done uploading` 후 → **IO0–GND 점퍼 제거** → RESET 한 번 더 눌러 일반 부팅

## 4. Arduino IDE 세팅

1. `File → Preferences → Additional Boards Manager URLs` 에 추가:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. `Tools → Board → Boards Manager` 에서 **esp32 (by Espressif Systems)** 설치
   - 이 저장소 권장 버전: **2.0.17** (3.x는 빌드 캐시 문제 + 얼굴 인식 미지원 — [`troubleshooting.md`](troubleshooting.md) 참고)
3. `Tools → Board` → **AI Thinker ESP32-CAM** 선택
4. `Tools` 메뉴 권장값:

| 항목 | 값 |
|---|---|
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| PSRAM | Enabled |
| Upload Speed | 115200 ※ |
| Port | FTDI가 잡힌 COMx / /dev/ttyUSBx |

> ※ 원문 가이드는 업로드 속도를 따로 강조하지 않지만, 이 저장소 실전에서는 460800/921600 사용 시 `Invalid head of packet` 에러가 잦아 115200을 권장.

## 5. CameraWebServer 예제 — 실시간 스트리밍 서버

공식 예제 하나로 스트리밍 + 카메라 설정 웹 UI까지 전부 됩니다.

### 5-1. 예제 열기와 수정

1. `File → Examples → ESP32 → Camera → CameraWebServer`
2. 카메라 모델 선택 — **`CAMERA_MODEL_AI_THINKER` 한 줄만** 주석 해제, 나머지 모델은 전부 `//` 주석:
   ```cpp
   //#define CAMERA_MODEL_WROVER_KIT
   //#define CAMERA_MODEL_ESP_EYE
   // ...
   #define CAMERA_MODEL_AI_THINKER   // ← 이것만 활성화
   ```
3. Wi-Fi 정보 입력 (**2.4GHz SSID**만 가능):
   ```cpp
   const char* ssid     = "네트워크이름";
   const char* password = "비밀번호";
   ```
4. 3장의 절차대로 업로드 (IO0–GND → Upload → 점퍼 제거 → RESET)

### 5-2. 접속

1. 시리얼 모니터를 **115200** baud로 열기
2. 부팅 로그에 다음처럼 출력됨:
   ```
   WiFi connected
   Camera Ready! Use 'http://192.168.x.x' to connect
   ```
3. 같은 공유기에 붙은 PC/폰 브라우저에서 그 주소로 접속
4. 왼쪽 설정 패널 + **Start Stream** 버튼이 있는 페이지가 열림
   - 스트림 자체는 `http://<IP>:81/stream` (포트 **81**) 로 서비스됨 — `<img>` 태그 등으로 직접 참조 가능

## 6. 웹 UI 비디오/카메라 설정 항목

Start Stream을 누르면 MJPEG 스트림이 시작되고, 왼쪽 패널의 항목들은 **스트리밍 중 실시간으로** 반영됩니다.

### 해상도·화질

| 항목 | 범위 | 설명 |
|---|---|---|
| Resolution | QQVGA(160×120) ~ UXGA(1600×1200) | 높일수록 화질↑ / FPS·반응속도↓. 스트리밍은 VGA(640×480)~SVGA(800×600)가 무난 |
| Quality | 10 ~ 63 | JPEG 압축 품질. **숫자가 작을수록 고화질**(대역폭↑). 10~12는 고해상도에서 프레임 깨질 수 있음 |

### 화면 조정

| 항목 | 범위 | 설명 |
|---|---|---|
| Brightness | -2 ~ 2 | 밝기 |
| Contrast | -2 ~ 2 | 대비 |
| Saturation | -2 ~ 2 | 채도 |
| Special Effect | 7종 | No Effect / Negative / Grayscale / Red·Green·Blue Tint / Sepia |
| H-Mirror / V-Flip | on/off | 좌우 반전 / 상하 반전 — 보드 장착 방향에 맞춰 사용 |

### 화이트밸런스·노출·게인 (기본값으로도 대부분 OK)

| 항목 | 설명 |
|---|---|
| AWB / AWB Gain | 자동 화이트밸런스. 끄면 WB Mode에서 Sunny / Cloudy / Office / Home 수동 선택 |
| AEC Sensor / AEC DSP | 자동 노출. 끄면 Exposure 값(0~1200) 수동 지정 |
| AE Level | -2 ~ 2, 자동 노출 보정치 |
| AGC / Gain | 자동 게인. 끄면 1x~31x 수동. Gain Ceiling으로 상한(2x~128x) 지정 — 어두운 곳에서 밝게 하는 대신 노이즈↑ |
| BPC / WPC | 불량(암/백) 픽셀 보정 |
| Raw GMA | 감마 보정 |
| Lens Correction | 렌즈 주변부 왜곡/음영 보정 |
| Color Bar | 테스트 컬러바 출력 (카메라 신호 자체 점검용) |

### 촬영·얼굴 기능

| 항목 | 설명 |
|---|---|
| Get Still | 현재 프레임을 정지 사진으로 캡처해 브라우저에 표시 (우클릭 저장) |
| Start / Stop Stream | MJPEG 스트리밍 시작/정지 |
| Face Detection | 얼굴 검출 박스 표시. **CIF(400×296) 이하 해상도에서만** 동작, esp32 코어 2.x 필요 |
| Face Recognition + Enroll Face | 등록(Enroll)한 얼굴 구분 인식. 자세한 실습은 [`../07_face_recognition/`](../07_face_recognition/) 참고 |

## 7. micro SD 카드로 사진 저장 (요약)

CameraWebServer와 별개로, 셔터 없이 부팅할 때마다 사진을 찍어 SD에 저장하는 패턴:

- SD는 `SD_MMC` 라이브러리로 접근 (SPI가 아니라 SDMMC 방식 — 별도 배선 불필요, 보드 슬롯 사용)
- 촬영 흐름: `esp_camera_init()` → `esp_camera_fb_get()` 으로 프레임 획득 → `SD_MMC.open(path, FILE_WRITE)` → `file.write(fb->buf, fb->len)` → `esp_camera_fb_return(fb)`
- 사진 번호는 EEPROM(또는 Preferences)에 저장해 부팅 간 이어가기
- **주의**: SD 4-bit 모드 사용 시 GPIO 4(플래시 LED)가 점유되어 촬영 때마다 LED가 번쩍임. `SD_MMC.begin("/sdcard", true)` 처럼 **1-bit 모드**로 열면 GPIO 4, 12, 13이 자유로워짐
- SD 카드는 FAT32 포맷 권장

예제 코드는 Random Nerd Tutorials의 [Take Photo and Save to MicroSD Card](https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/) 참고.

## 8. 자주 막히는 부분 (원문 + 실전)

| 증상 | 해결 |
|---|---|
| `Failed to connect to ESP32: Timed out...` | IO0–GND 점퍼 확인, `Connecting...` 중 RESET 누르기 |
| 업로드는 됐는데 아무 동작 없음 | IO0–GND 점퍼 **제거 후** RESET 눌렀는지 확인 |
| `Brownout detector was triggered` | 전원 부족 — 5V 공급, 짧고 굵은 케이블, USB 허브 금지 |
| 카메라 init 실패 / 조용히 멈춤 | `CAMERA_MODEL_AI_THINKER`만 활성화됐는지, PSRAM Enabled인지 확인 |
| Wi-Fi 연결 안 됨 | 5GHz 전용 SSID 아닌지 확인 (2.4GHz만 지원) |
| 스트림이 자주 끊김 | 해상도/Quality 낮추기, 공유기와 거리 줄이기 |

더 많은 사례는 [`troubleshooting.md`](troubleshooting.md) 참고.
