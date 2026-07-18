# 3단계 — Wi-Fi 연결 & 시각 동기화

ESP32의 Wi-Fi 기본기를 익히는 단계. 카메라 스트리밍 전에 네트워크가 잘 붙는지 먼저 확인합니다.

## 목표
- 2.4GHz Wi-Fi에 접속
- 공유기에서 IP 받기 → 시리얼에 출력
- NTP 서버에서 한국 시각 가져와서 1초마다 출력

> ⚠ ESP32는 **2.4GHz** 만 지원합니다. 5GHz 전용 SSID는 안 보임.

## 파일
- `wifi_time.ino` — Wi-Fi 접속 + NTP

## 사용 전 수정할 곳
```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

업로드 후 시리얼 모니터(115200)에서 IP와 시간이 보이면 성공.

## 실패하면
- SSID/PW 오타
- 2.4GHz 신호 약함 (안테나 방향, 거리)
- 한국에서 NTP는 `kr.pool.ntp.org` 가 빠름
