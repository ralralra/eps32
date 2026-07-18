# 7단계 — 얼굴 검출 & 얼굴 인식 (CameraWebServer 내장 기능)

5단계에서 띄운 `CameraWebServer` 웹UI에는 사실 얼굴 **검출(Detection)** 과 **인식(Recognition)** 기능이 숨겨져 있습니다.
이 단계는 별도 코드를 작성하지 않고, 그 UI를 켜고 등록(Enroll)/인식까지 따라가는 연습입니다.

참고 가이드: https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/

## ⚠ 매우 중요 — 보드 코어 버전 확인

| esp32 보드 코어 버전 | CameraWebServer에 얼굴 검출 | 얼굴 인식(Enroll/Recognition) |
|---|---|---|
| **2.x (2.0.x)** | ✅ 있음 | ✅ 있음 (이 단계 그대로 진행 가능) |
| **3.x 이상** | ✅ 검출은 남음 | ❌ esp-face 라이브러리 단종으로 **제거됨** |

> `Tools → Board → Boards Manager → esp32` 에서 **버전을 2.0.17 등 2.x로 다운그레이드**해야 얼굴 인식 토글이 보입니다.
> 3.x를 유지하면서 인식을 하려면 ESP-WHO / TensorFlow Lite Micro 같은 별도 솔루션이 필요 — 그건 `projects/`의 ★★★ 과제로 남겨둠.

## 절차 (코어 2.x 가정)

### 1. CameraWebServer 빌드 옵션
- Board: **AI Thinker ESP32-CAM**
- Partition Scheme: **Huge APP (3MB No OTA / 1MB SPIFFS)** ← 꼭 필요
- PSRAM: **Enabled**

### 2. 코드 수정
`CameraWebServer.ino`:
```cpp
#define CAMERA_MODEL_AI_THINKER      // 주석 해제
// #define CAMERA_MODEL_WROVER_KIT    // 나머지는 주석
// ... 등등
const char* ssid     = "YOUR_WIFI";
const char* password = "YOUR_PASS";
```

### 3. 업로드 → 브라우저 접속
시리얼에 찍힌 IP로 접속.

### 4. 얼굴 검출 켜기
좌측 컨트롤 패널 아래쪽:
1. **Resolution** → `CIF (400x296)` 또는 `QVGA (320x240)` 권장 (얼굴 인식은 작은 해상도에서만 동작)
2. **Face Detection** 토글 ON
3. **Start Stream** 클릭 → 얼굴에 노란 박스가 그려지면 성공

### 5. 얼굴 등록 (Enroll Face)
1. **Face Recognition** 토글 ON
2. **Enroll Face** 버튼 클릭
3. 카메라 정면을 보고 잠시 대기 → 박스가 초록색 + 사용자ID(예: `Subject 0`) 표시되면 등록 완료
4. 새 얼굴을 등록하려면 다른 사람이 카메라 앞에 서고 다시 **Enroll Face** 클릭

> 내장 인식은 일반적으로 **최대 7명**까지 등록 가능. 이상은 덮어쓰기.

### 6. 인식 동작
- 등록된 얼굴: 초록 박스 + `Subject N` 라벨
- 모르는 얼굴: 빨간 박스 + `Intruder Alert` 라벨

## 연습 과제
- [ ] 자기 얼굴 1명 등록 → 라벨이 일관되게 떠야 함
- [ ] 2명 등록 → 둘이 동시에 카메라 앞에 섰을 때 라벨이 잘 매칭되는지
- [ ] 안경/마스크/조명을 바꿔 보고 인식 성공률 메모
- [ ] 해상도를 `VGA`로 올리면 어떻게 되는지 (느려지거나 인식 꺼짐)

## 자주 막히는 곳
- **얼굴 인식 토글이 안 보임** → 코어가 3.x. 2.0.17로 다운그레이드 필요
- **빌드 에러 `text section exceeds available space`** → 파티션이 Default 상태. **Huge APP**로 변경
- **얼굴 검출이 항상 박스 X** → 해상도 너무 큼. QVGA/CIF로 낮춤
- **인식이 자꾸 다른 사람으로 판정** → 등록 시 정면/밝은 조명에서 여러 번 Enroll, 그래도 7명 캐파의 한계가 큼

## 발전 방향
RNT 튜토리얼이 보여주는 내장 인식은 데모로 충분하지만 정확도/캐파가 약합니다. 더 가고 싶다면:
- **ESP-WHO** 직접 사용 (코어 3.x + IDF 환경) — 임베딩 추출 후 직접 매칭
- 얼굴 인식은 PC/서버로 보내고 ESP32-CAM은 캡처만 담당 (현실적으로 가장 잘 됨)

→ `projects/` 의 ★★★ 얼굴 인식 출입 로그 항목으로 이어집니다.
