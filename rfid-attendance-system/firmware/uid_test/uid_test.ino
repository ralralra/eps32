/*
  0단계 — RC522 배선 확인 + 학생증 UID 읽기 테스트
  ----------------------------------------------------
  목표: 학생증을 리더기에 대면 시리얼 모니터(115200)에 UID가 찍힌다.

  이걸로 확인하는 것:
    1) 배선이 맞는지 (docs/wiring.md 참고)
    2) 우리 학교 학생증이 RC522로 읽히는 카드인지 (ISO 14443 Type A)
       → 안 찍히면 Type B/FeliCa 카드일 수 있음. 다른 카드로 테스트해 볼 것.

  준비: Arduino IDE → 라이브러리 매니저 → "MFRC522" (by GithubCommunity) 설치
  보드: ESP32 Dev Module (WROOM-32)
*/

#include <SPI.h>
#include <MFRC522.h>

#define PIN_SS   5    // RC522 SDA(SS)
#define PIN_RST  27   // RC522 RST

MFRC522 rfid(PIN_SS, PIN_RST);

void setup() {
  Serial.begin(115200);
  SPI.begin();          // SCK=18, MISO=19, MOSI=23 (ESP32 기본 SPI 핀)
  rfid.PCD_Init();
  delay(100);
  // 안테나 감도를 최대로 (기본값은 중간 — 클론 모듈은 이걸 안 올리면 인식 거리가 매우 짧음)
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);

  // 리더기가 응답하는지 자가진단 (배선이 틀리면 0x00 또는 0xFF가 나옴)
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.println();
  Serial.print("RC522 버전 레지스터: 0x");
  Serial.println(version, HEX);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("⚠️ 리더기 응답 없음! 배선을 다시 확인하세요 (특히 3.3V, SDA=5, RST=27)");
  } else {
    Serial.println("✅ 리더기 연결 OK. 학생증을 대보세요.");
  }
}

void loop() {
  // 새 카드가 올라왔고, UID를 읽을 수 있으면
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // UID를 "A1B2C3D4" 처럼 대문자 16진수 문자열로 만든다
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("카드 인식! UID = ");
  Serial.print(uid);
  Serial.print("  (");
  Serial.print(rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak)));
  Serial.println(")");

  rfid.PICC_HaltA();        // 카드와의 통신 종료
  rfid.PCD_StopCrypto1();
  delay(1000);              // 같은 카드 연속 인식 방지
}
