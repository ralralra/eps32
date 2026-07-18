# 4단계 — 카메라 한 장 찍기

OV2640 카메라를 초기화하고 JPEG 한 장을 캡처해서 정보(해상도/크기)를 시리얼에 출력해 봅니다.
이 단계가 성공하면 카메라/PSRAM/전원 모두 정상.

## 핀맵 (AI Thinker ESP32-CAM)
`camera_pins.h`에 매크로로 정의돼 있어요. 직접 외우진 마세요.

| 신호 | GPIO |
|---|---|
| PWDN | 32 |
| RESET | -1 (미사용) |
| XCLK | 0 |
| SIOD (SDA) | 26 |
| SIOC (SCL) | 27 |
| Y9..Y2 (D7..D0) | 35,34,39,36,21,19,18,5 |
| VSYNC | 25 |
| HREF | 23 |
| PCLK | 22 |

> ⚠ XCLK는 GPIO0 — 업로드용 부트핀과 공유. 그래서 업로드할 때는 GND, 동작할 때는 떼는 것.

## 파일
- `camera_pins.h` — 핀 정의
- `camera_basic.ino` — 5초마다 한 장 찍고 정보 출력

## 보드 설정 다시 확인
- `Tools` → `PSRAM` → **Enabled** (필수, 안 켜면 큰 해상도에서 메모리 부족)
- `Tools` → `Partition Scheme` → **Huge APP**

## 동작
업로드 후 시리얼에서 이런 메시지가 나오면 성공:
```
JPEG captured: 640x480, 18342 bytes, fb_count=2
```

## 자주 막히는 곳
- `Camera init failed with error 0x20004` → PSRAM 비활성. 보드 설정 다시 확인
- `Brownout detector was triggered` → 전원 부족. 5V/2A 외부 어댑터 권장
- 화면이 보라색/녹색 줄무늬 → PCLK 노이즈. 점퍼선 짧게, 흔들림 줄이기
