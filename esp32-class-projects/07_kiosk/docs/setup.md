# 키오스크 설정 (Wemos D1 R32)

> 보드 처음 세팅(드라이버·IDE·보드 패키지): [`01_docs/setup_wemos_d1_r32.md`](../../01_docs/setup_wemos_d1_r32.md)

## 회로 — 없음!
Wemos D1 R32 + USB 케이블만 있으면 됩니다. 추가 부품·배선 없음.
화면은 폰/태블릿 브라우저예요. (태블릿이 키오스크 느낌이 더 납니다)

## 보드 설정
- Board: **ESP32 Dev Module**
- Upload Speed: 115200
- Partition Scheme: **Default 4MB with spiffs** (3·4단계 저장용)

## 접속
1. 업로드 후 시리얼에 `http://192.168.4.1` 확인
2. 폰/태블릿 와이파이 `ESP32-Kiosk` 접속 (비번 `kiosk1234`)
3. 브라우저에서 **http://192.168.4.1**

## 포인트/매출 초기화
- 매출: `/sales` → "매출 초기화"
- 포인트 잔액 초기화: SPIFFS의 `/balance.txt` 삭제가 필요. 간단히는
  업로드 시 `Tools → Erase All Flash Before Sketch Upload: Enabled` 로 한 번 지우면 시작 포인트(10000)로 리셋됩니다.

## 문제 해결
웹 접속/Brownout 등은 출석 프로젝트의 [`04_attendance/docs/troubleshooting.md`](../../04_attendance/docs/troubleshooting.md) 와 동일합니다.
