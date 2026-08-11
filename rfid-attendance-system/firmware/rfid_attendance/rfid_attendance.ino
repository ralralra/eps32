/*
  스마트 출석체크 — ESP32 완성 펌웨어
  ----------------------------------------------------
  동작:
    1) 학교 와이파이에 접속한다
    2) 2초마다 중계서버(Apps Script)에 "할 일 있나요?" 하고 물어본다 (폴링)
    3) 앱에서 출석체크(또는 등록)를 시작하면 → LED가 깜빡이며 카드 대기
       - 출석 모드: 천천히 깜빡 (0.5초)
       - 등록 모드: 빠르게 깜빡 (0.15초)
    4) 학생증을 태그하면 UID를 중계서버로 보내고, 결과에 따라 부저가 울린다
       - 성공: 삑삑 (짧게 2번)
       - 실패(미등록/카드 불일치 등): 삐— (길게 1번)

  준비:
    - 라이브러리: "MFRC522" (by GithubCommunity)
    - 아래 [설정] 4줄을 자기 것으로 바꾸기
    - 중계서버 배포 방법: apps_script/README.md
    - 배선: docs/wiring.md  (RC522는 반드시 3.3V!)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ── [설정] 여기 4줄만 바꾸면 됩니다 ─────────────────────────
const char* WIFI_SSID  = "와이파이이름";
const char* WIFI_PASS  = "와이파이비밀번호";
const char* RELAY_URL  = "https://script.google.com/macros/s/여기에배포ID/exec";
const char* DEVICE_ID  = "DEV001";   // 앱에 표시되는 리더기 이름 (1-3반 입구)
// ──────────────────────────────────────────────────────────

// 핀 배치 (docs/wiring.md 와 같음)
#define PIN_SS      5    // RC522 SDA(SS)
#define PIN_RST     27   // RC522 RST
#define PIN_LED     2    // 보드 내장 파란 LED
#define PIN_BUZZER  4    // 수동부저 (없으면 안 달아도 됨 — 소리만 안 남)

const unsigned long POLL_MS = 2000;   // 폴링 간격 (2초)

MFRC522 rfid(PIN_SS, PIN_RST);

// 현재 상태: IDLE(대기) / ATTEND(출석 세션) / REGISTER(등록 세션)
String mode = "IDLE";
unsigned long lastPoll = 0;
unsigned long lastBlink = 0;
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  SPI.begin();          // SCK=18, MISO=19, MOSI=23
  rfid.PCD_Init();
  delay(100);
  // 안테나 감도를 최대로 (기본값은 중간 — 인식 거리가 짧으면 필수)
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  // 송신 출력 부스트 — 안테나가 작은 카드(학생증·유스카드 등)도 깨울 수 있게
  // 반송파 구동 세기를 최대로 올린다 (uid_test에서 검증된 설정)
  rfid.PCD_WriteRegister(MFRC522::GsNReg, 0xF8);    // 기본 0x88
  rfid.PCD_WriteRegister(MFRC522::CWGsPReg, 0x3F);  // 기본 0x20

  Serial.println();
  Serial.print("와이파이 접속 중");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("접속 완료! IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("앱에서 [출석체크 시작]을 누르면 LED가 깜빡입니다.");
  beep(80); beep(80);   // 준비 완료 알림
}

void loop() {
  // 와이파이가 끊기면 재접속
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("와이파이 끊김 → 재접속");
    WiFi.reconnect();
    delay(2000);
    return;
  }

  // ① 주기적으로 중계서버에 할 일이 있는지 물어본다
  if (millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    String res = httpGet(String(RELAY_URL) + "?action=poll&device=" + DEVICE_ID);
    if (res == "ATTEND" || res == "REGISTER" || res == "IDLE") {
      if (res != mode) {
        Serial.println("모드 변경: " + mode + " → " + res);
        if (res == "IDLE") digitalWrite(PIN_LED, LOW);
      }
      mode = res;
    }
  }

  // ② 세션 중이면 LED 깜빡이기 (출석: 0.5초 / 등록: 0.15초)
  if (mode != "IDLE") {
    unsigned long interval = (mode == "REGISTER") ? 150 : 500;
    if (millis() - lastBlink >= interval) {
      lastBlink = millis();
      ledOn = !ledOn;
      digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    }
  }

  // ③ 카드가 태그되면: 세션 중이면 서버로 전송, 대기 중이면 확인용으로 UID만 출력
  if (cardTagged()) {
    String uid = readUid();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    if (mode == "IDLE") {
      // 세션이 없을 때도 카드가 잘 읽히는지 시리얼로 확인할 수 있게 출력만 한다
      Serial.println("카드 인식 (세션 없음 → 전송 안 함): UID = " + uid);
      delay(1000);               // 같은 카드 연속 인식 방지
      return;
    }

    Serial.println("태그됨! UID = " + uid + " → 서버 전송");
    digitalWrite(PIN_LED, HIGH);

    String res = httpGet(String(RELAY_URL) + "?action=tag&device=" + DEVICE_ID + "&uid=" + uid);
    handleTagResult(res);

    mode = "IDLE";               // 처리 끝 → 대기 상태로
    digitalWrite(PIN_LED, LOW);
    lastPoll = millis();         // 방금 통신했으니 다음 폴링은 2초 뒤
    delay(1000);                 // 같은 카드 연속 인식 방지
  }
}

// 카드가 리더기 위에 있는지 WUPA(깨우기) 방식으로 확인
// 학생증 같은 보안 스마트카드(ISO 14443-4)는 일반 감지(REQA)에 응답하지 않는
// 경우가 있어서 WUPA 방식이 훨씬 안정적으로 잡는다.
bool cardTagged() {
  byte atqa[2];
  byte atqaSize = sizeof(atqa);
  MFRC522::StatusCode st = rfid.PICC_WakeupA(atqa, &atqaSize);
  if (st != MFRC522::STATUS_OK && st != MFRC522::STATUS_COLLISION) return false;
  return rfid.PICC_ReadCardSerial();
}

// UID를 "A1B2C3D4" 형태의 대문자 16진수 문자열로
String readUid() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// 서버 응답에 따라 소리·메시지로 알려주기
// 성공: "OK,이름,출석,08:55:01" 또는 "OK_REG,이름"
// 실패: "UNKNOWN"(미등록 카드) / "DUP,이름"(이미 등록된 카드) / "MISMATCH,이름"(다른 학생 카드)
void handleTagResult(String res) {
  if (res.startsWith("OK_REG")) {
    Serial.println("✅ 등록 완료! → " + res);
    beep(80); beep(80);
  } else if (res.startsWith("OK")) {
    Serial.println("✅ 출석 확인! → " + res);
    beep(80); beep(80);
  } else if (res.startsWith("UNKNOWN")) {
    Serial.println("❌ 등록되지 않은 카드입니다. 먼저 앱에서 등록하세요.");
    beep(600);
  } else if (res.startsWith("DUP")) {
    Serial.println("❌ 이미 다른 학생으로 등록된 카드입니다 → " + res);
    beep(600);
  } else if (res.startsWith("MISMATCH")) {
    Serial.println("❌ 대상 학생의 카드가 아닙니다 → " + res);
    beep(600);
  } else {
    Serial.println("⚠️ 서버 응답: " + res);
    beep(600);
  }
}

// HTTPS GET 요청 → 응답 본문(문자열) 반환
// Apps Script는 응답할 때 주소를 한 번 이동(redirect)시키므로 따라가기 설정이 꼭 필요
String httpGet(String url) {
  WiFiClientSecure client;
  client.setInsecure();   // 수업용: 인증서 검증 생략 (통신 자체는 HTTPS 암호화됨)

  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.setTimeout(10000);

  String body = "";
  if (https.begin(client, url)) {
    int code = https.GET();
    if (code == HTTP_CODE_OK) {
      body = https.getString();
      body.trim();
    } else {
      Serial.println("HTTP 오류: " + String(code));
    }
    https.end();
  }
  return body;
}

// 수동부저 삑 소리 (ms 만큼)
// 2kHz 사각파를 직접 만든다 — ESP32 코어 버전과 관계없이 동작
void beep(int ms) {
  long cycles = (long)ms * 2;          // 2kHz → 한 주기 0.5ms
  for (long i = 0; i < cycles; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(250);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(250);
  }
  delay(60);
}
