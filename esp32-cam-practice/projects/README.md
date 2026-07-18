# 미니 프로젝트 아이디어

기초 6단계가 끝났다면 다음 중 하나를 골라 완성해 보세요. 난이도순.

## ★ 1. 타임랩스 카메라
- 30초 ~ 5분 간격으로 캡처 → SD 카드에 저장
- 파일명은 NTP 시각 기반 (`2026-06-27_18-30-00.jpg`)
- 외부 5V 어댑터 + 케이스에 넣고 식물/창밖 며칠 촬영
- 학습 포인트: `SD_MMC` 라이브러리, RTC, deep sleep으로 배터리 절약

## ★★ 2. 텔레그램 봇 도어카메라
- 5단계 + 6단계 합치기
- 움직임 감지되면 JPEG 캡처 → 텔레그램 봇으로 푸시 전송
- 챗에서 `/photo` 보내면 즉시 한 장 찍어 회신
- 학습 포인트: HTTPS 클라이언트, multipart/form-data 업로드, 봇 토큰 보안

## ★★ 3. MQTT + Home Assistant
- 카메라를 Home Assistant의 MJPEG/Snapshot 카메라로 등록
- 움직임 이벤트를 MQTT 토픽에 publish → HA 오토메이션 트리거
- 학습 포인트: `PubSubClient`, HA의 MJPEG IP Camera 통합

## ★★★ 4. 얼굴 인식 출입 로그
- esp-face / TensorFlow Lite Micro 로 얼굴 임베딩
- 등록된 얼굴이면 OK, 모르면 사진과 함께 알림
- 학습 포인트: ESP-WHO 예제 기반, 메모리/속도 한계 체감

## ★★★ 5. WebRTC 저지연 스트리밍
- 기본 MJPEG 대신 WebRTC(esp_webrtc 라이브러리)로 ~200ms 지연 영상
- 학습 포인트: SDP, STUN, NAT 통과

---

각 프로젝트는 하위 폴더를 새로 만들어 `README.md`, `main.ino`, 회로 사진 정도로 정리하면 좋습니다.
