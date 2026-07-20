# 셀 샘플 모음 — 연결 그림 + 바로 쓰는 코드

> 고릴라셀을 실드에 꽂고 **그림대로 연결 → 코드 업로드 → 시리얼(115200) 관찰**.
> 핀 번호는 **꽂은 자리의 GPIO**로 수정하세요 — [실드 전체 핀맵](../images/gorilla_shield_x_d1r32_pinmap.png) · [핀맵 문서](../01_docs/pinmap_wemos_d1_r32.md)

## 연결 그림 (포트별)

| 그림 | 대상 셀 | 꽂는 곳 |
|---|---|---|
| **[아날로그 셀](../images/wiring_analog_cell.png)** | 빛 · 소리 · 토양습도 | **Analog 블록 A2~A5 위치** (35·34·36·39) |
| **[디지털 셀](../images/wiring_digital_cell.png)** | 진동 · 홀(자석) · 온습도(DHT) | Digital I/O 블록 아무 포트 |
| **[초음파](../images/wiring_ultrasonic.png)** | 초음파 거리 (TRIG·ECHO) | Digital 포트 1개 (신호 2칸) |
| **[7세그먼트](../images/wiring_7segment.png)** | TM1637 숫자표시기 (DIO·CLK) | Digital 포트 1개 (신호 2칸) |
| [LCD — 옵션](../images/lcd_wiring_gorilla_shield.png) | I2C LCD (16×2) — **본 과정 미사용** | ⚠ 실드 I2C 블록에서 동작하지 않을 수 있어요 (아래 참고) |

## 샘플 코드

| # | 스케치 | 셀 | 핵심 함수 | 라이브러리 |
|:---:|---|---|---|---|
| 00 | [`00_i2c_scanner`](00_i2c_scanner/00_i2c_scanner.ino) | **I2C 스캐너** — I2C 장치 주소·연결 진단 | `Wire` | — |
| 01 | [`01_light`](01_light/01_light.ino) | 빛 | `analogRead` | — |
| 02 | [`02_sound`](02_sound/02_sound.ino) | 소리 | `analogRead` | — |
| 03 | [`03_soil`](03_soil/03_soil.ino) | 토양습도 (+% 변환) | `analogRead` + `map` | — |
| 04 | [`04_vibration`](04_vibration/04_vibration.ino) | 진동 | `digitalRead` | — |
| 05 | [`05_hall`](05_hall/05_hall.ino) | 홀(자석) — 문 열림 | `digitalRead` | — |
| 06 | [`06_dht`](06_dht/06_dht.ino) | 온습도 | `readTemperature/Humidity` | DHT sensor library |
| 07 | [`07_ultrasonic`](07_ultrasonic/07_ultrasonic.ino) | 초음파 거리 | `pulseIn` | — |
| 08 | [`08_7segment`](08_7segment/08_7segment.ino) | 7세그먼트 기본 | `showNumberDecEx` | **TM1637** (Avishay Orpaz) |
| 09 | [`09_7segment_timer`](09_7segment_timer/09_7segment_timer.ino) | 7세그 **카운트다운 타이머** | 분:초 표시 + 깜빡임 | TM1637 |
| 10 | [`10_lcd_countdown`](10_lcd_countdown/10_lcd_countdown.ino) | (옵션) LCD 카운트다운 — **본 과정 미사용**, 수업용은 06·09! | `lcd.print` | LiquidCrystal I2C |
| 11 | [`11_openmeteo_weather`](11_openmeteo_weather/11_openmeteo_weather.ino) | **바깥 날씨 API** — 안팎 온도 비교 (4회차 심화) | HTTPS GET + JSON | TM1637 · DHT |

## 공통 규칙 3가지

1. **아날로그 셀은 A2~A5 위치** — A0·A1 자리(GPIO 2·4)는 WiFi를 켜면 아날로그를 못 읽어요
2. 코드의 `#define` 번호는 실드에 인쇄된 숫자가 아니라 **변환된 GPIO** (예: '6' 칸 = GPIO27)
3. 신호 2개짜리 셀(초음파·7세그)은 **한 포트의 신호 2칸을 통째로** — 순서 바뀌면 동작 안 해요

> ⚠ **LCD(I2C)는 본 과정에서 사용하지 않아요.** 이 실드의 I2C 블록이 우노 A4·A5 라인 기준이면
> D1 R32에서는 GPIO36·39(입력 전용)로 이어져 I2C가 동작하지 않습니다.
> 굳이 쓰려면 디지털 '6'·'7' 칸에 꽂고 코드에 `Wire.begin(27, 14);` (SDA→'6', SCL→'7') — 주소 확인은 00번 스캐너로!
