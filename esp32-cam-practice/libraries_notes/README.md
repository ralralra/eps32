# 사용/참고 라이브러리 메모

연습 중에 설치한 라이브러리와 버전, 한 줄 메모를 적어 두면 다음 세션에 재현하기 쉬움.

| 라이브러리 | 버전 | 용도 | 비고 |
|---|---|---|---|
| esp32 (Boards Manager) | **2.0.17** (★ 권장) | ESP32 전체 환경 | `esp_camera`/`esp_http_server` 포함. 3.x는 빌드 캐시 깨짐 + CameraWebServer 얼굴 인식 없음. **2.0.17 고정 권장** |
| (필요 시) ArduinoJson | 7.x | JSON 파싱 | 텔레그램/HA 연동 시 |
| (필요 시) UniversalTelegramBot | 1.3.x | 텔레그램 봇 | 도어카메라 프로젝트용 |
| (필요 시) PubSubClient | 2.8 | MQTT | HA 연동용 |

> Arduino Library Manager (`Sketch` → `Include Library` → `Manage Libraries`)에서 검색해 설치.

## 보드 매니저 URL

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
