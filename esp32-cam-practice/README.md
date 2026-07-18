# ESP32-CAM 학습 공간

ESP32-CAM (OV2640, 4MB PSRAM) 보드를 단계별로 익히고 작은 프로젝트까지 만들어 보는 연습 저장소입니다.

## 보드 정보

- **MCU**: ESP32-S (Tensilica Xtensa LX6, 듀얼코어 240MHz)
- **카메라**: OV2640 (최대 1600x1200, JPEG/RGB565/YUV422)
- **PSRAM**: 4MB (고해상도 캡처 시 필수)
- **Flash**: 4MB
- **무선**: Wi-Fi 802.11 b/g/n + Bluetooth Classic/BLE
- **GPIO**: 10개 (다수는 카메라/SD에 점유) — 자유 핀이 적음
- **주의**: USB 포트가 **없음**. 업로드는 외장 USB-UART(예: FTDI, CP2102) 또는 ESP32-CAM-MB 어댑터 필요

## 학습 단계

| 단계 | 폴더 | 목표 |
|---|---|---|
| 1 | `01_arduino_ide_setup` | Arduino IDE 설치, ESP32 보드 매니저 추가, 드라이버 |
| 2 | `02_first_blink` | 보드 인식 확인, 첫 업로드, 부트모드(GPIO0) 익히기 |
| 3 | `03_wifi_connect` | Wi-Fi 연결, NTP 시각 가져오기, Serial 디버깅 |
| 4 | `04_camera_basic` | 카메라 초기화, JPEG 한 장 캡처 → Serial 출력 |
| 5 | `05_camera_web_server` | 브라우저로 실시간 스트리밍 (공식 CameraWebServer) |
| 6 | `06_motion_detection` | 프레임 차분으로 움직임 감지 → 사진 저장/알림 |
| 7 | `07_face_recognition` | CameraWebServer 내장 얼굴 검출/인식 (Enroll Face) — RNT 튜토리얼 따라가기 |
| ★ | `projects` | 완성형 미니 프로젝트 (예: 도어카메라, 타임랩스, 텔레그램 봇) |

## 외부 참고 가이드

| 링크 | 어느 단계에 좋은가 |
|---|---|
| [Random Nerd Tutorials — ESP32-CAM Video Streaming and Face Recognition](https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/) | 5단계 + 7단계. 스크린샷이 풍부함. 얼굴 인식은 esp32 코어 **2.x** 필수 |
| [Espressif arduino-esp32 — Camera 예제](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera) | 원본 예제 모음 |
| [Random Nerd Tutorials — ESP32-CAM 시리즈 인덱스](https://randomnerdtutorials.com/projects-esp32-cam/) | 타임랩스/SD 저장/PIR 모션 등 응용 예제 다수 |

> 외부 가이드는 보드 코어 버전에 따라 빌드가 달라질 수 있으니 `docs/troubleshooting.md` 도 함께 참고.

## 환경 권장값 (실전에서 검증)

| 항목 | 값 | 이유 |
|---|---|---|
| esp32 보드 코어 | **2.0.17** | 3.x 는 빌드 캐시 깨짐(`undefined reference to 'Serial0'` 류), 얼굴 인식 단종 |
| Board | AI Thinker ESP32-CAM | PSRAM 자동 설정, 카메라 핀 일치 |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) | 카메라/얼굴인식 코드 크기 |
| PSRAM | Enabled | 고해상도 캡처 필수 |
| Upload Speed | **115200** | 460800/921600 은 `Invalid head of packet` 잘 남 |
| Erase All Flash Before Sketch Upload | Disabled (평소) | 매번 켜면 업로드 길어지고 깨질 확률↑ |
| Serial Monitor baud | 115200 | 코드의 `Serial.begin(115200)` 과 일치 |

## 폴더 안내

- `docs/` — 핀맵, 자주 쓰는 카메라 설정값, 트러블슈팅
- `libraries_notes/` — 사용한 라이브러리와 버전 정리

## 진행 방식

각 단계 폴더 안의 `README.md`를 먼저 읽고 → 회로 연결 → 코드 업로드 → 시리얼 모니터로 확인 → 막히는 부분 기록.

시작은 `01_arduino_ide_setup/README.md` 부터.
