/*
  5단계 — 스마트 보관함 (서보 4개 + 리드스위치 4개, 센서쉴드 버전)
  ----------------------------------------------------------------------
  슬롯마다 서보(잠금)와 리드스위치(우산 감지)가 짝을 이룹니다.
    리드스위치 감지(자석 가까이) = 우산 있음 / 감지 안 됨 = 우산 없음

  ● 대여: 열림 → 우산이 빠지면 5초 뒤 닫힘 → 확인
       우산 없음 → 대여완료
       30초가 지나도 안 가져가면 → 닫고 대여실패 + 서버에 대여 취소 요청
  ● 반납: 열림 → 우산이 꽂히면 닫힘 → 확인
       우산 감지 → 반납완료
       감지 안 됨 → 다시 열림 (최대 3회, 그래도 없으면 반납실패)

  기다리는 동안에도 멈추지 않습니다(논블로킹). 통신은 별도 코어에서 돌아가므로
  WiFi가 느려도 센서 감지와 5초 타이머가 밀리지 않습니다.

  명령 두 가지 경로
    ① 앱  : 서버 명령 큐를 1.5초마다 확인 ("RENT:1" / "RETURN:1")
    ② USB : 시리얼 모니터(115200)
            r1~r4 대여 · b1~b4 반납 · o1~o4 그냥 열기 · c1~c4 그냥 닫기 · s 상태

  배선: docs/wiring_servo_reed.md — 센서쉴드에 그대로 꽂으면 됩니다
    서보 신호: GPIO16·17·25·26 (실드 숫자 5·4·3·2)
    리드:      GPIO13·14·27·4  (실드 숫자 9·7·6·A1)
  ⚠ 서보 전원은 실드 외부 전원 단자에 5V — 12V를 넣으면 서보가 탑니다!

  라이브러리: ESP32Servo (기본 Servo.h는 ESP32에서 안 됩니다)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// ─────────────────────────────────────────────────────────────
// 1. 바꿀 곳
// ─────────────────────────────────────────────────────────────

// ⚠ WiFi 비밀번호를 넣은 채로 GitHub에 올리지 마세요!
const char WIFI_SSID[]     = "WiFi이름";
const char WIFI_PASSWORD[] = "비밀번호";

// 구글 시트 '시트1' 탭의 /exec 주소
const char SERVER_URL[] = "https://script.google.com/macros/s/XXXX/exec";

// 이 보드가 담당하는 보관함 — 시트 lockers 탭의 locker_id와 같아야 합니다.
// 슬롯 번호(1~4)는 보관함마다 겹치므로 이 값으로 구분합니다.
const char LOCKER_ID[] = "L001";

// ─────────────────────────────────────────────────────────────
// 2. 하드웨어 설정
// ─────────────────────────────────────────────────────────────

constexpr uint8_t SLOT_COUNT = 4;
constexpr uint8_t SERVO_PINS[SLOT_COUNT] = {16, 17, 25, 26};
constexpr uint8_t REED_PINS[SLOT_COUNT]  = {13, 14, 27, 4};

// 리드스위치를 GPIO-GND 사이에 연결하면 우산이 있을 때 LOW입니다.
// 반대로 동작하는 센서 모듈이면 HIGH로 바꾸세요.
constexpr uint8_t REED_ACTIVE_LEVEL = LOW;

// ⚠ 0/180은 쓰지 마세요. SG90은 양 끝에 물리적 스토퍼가 있어서, 끝단을 명령하면
//   부딪힌 채 계속 힘을 쓰는 상태(스톨)가 됩니다 — 전류가 최대로 올라가고 뜨거워져요.
constexpr int SERVO_CLOSED_ANGLE = 20;      // 잠김
constexpr int SERVO_OPEN_ANGLE   = 100;     // 열림
constexpr int SERVO_MIN_PULSE_US = 500;
constexpr int SERVO_MAX_PULSE_US = 2400;

// 다 움직인 서보의 힘을 빼둘지 — 전류·떨림·발열이 크게 줍니다.
// 문이 저절로 내려앉으면 false로 바꾸세요(대신 전원이 넉넉해야 합니다).
constexpr bool RELEASE_WHEN_IDLE = true;

// ─────────────────────────────────────────────────────────────
// 3. 시간 설정
// ─────────────────────────────────────────────────────────────

constexpr uint32_t RENT_CLOSE_DELAY_MS    = 5000;   // 우산이 빠진 뒤 닫기까지
constexpr uint32_t RETURN_PRESENT_HOLD_MS = 800;    // 꽂힌 상태가 이만큼 유지되면 닫기
constexpr uint32_t SERVO_MOVE_MS          = 900;    // 서보가 다 움직일 때까지
constexpr uint32_t REED_DEBOUNCE_MS       = 80;     // 접점 떨림 무시
constexpr uint8_t  RETURN_RETRY_MAX       = 3;      // 반납 재개방 최대 횟수

// 열어둔 채 아무 일도 없을 때 자동으로 닫는 시간 (0이면 자동 닫힘 없음)
// 부스에서 누가 누르고 그냥 가버려도 문이 계속 열려 있지 않게 합니다.
constexpr uint32_t OPEN_FAILSAFE_MS = 30000;

constexpr uint32_t COMMAND_POLL_MS = 1500;   // 서버 명령 확인 주기
constexpr uint32_t REPORT_EVERY_MS = 30000;  // 슬롯 상태 정기 보고
constexpr uint32_t WIFI_RETRY_MS   = 10000;
constexpr uint32_t HTTP_TIMEOUT_MS = 10000;

// ─────────────────────────────────────────────────────────────
// 4. 상태 정의
// ─────────────────────────────────────────────────────────────

enum class FlowState : uint8_t {
  LOCKED_IDLE,          // 잠긴 채 대기
  MANUAL_OPEN,          // o 명령으로 열어둔 상태
  RENT_WAIT_REMOVE,     // 대여: 열고 우산이 빠지기를 기다림
  RENT_VERIFY,          // 대여: 닫고 결과 확인
  RETURN_WAIT_INSERT,   // 반납: 열고 우산이 꽂히기를 기다림
  RETURN_WAIT_DONE,     // 반납(2단계): RETURN_DONE 명령을 기다림
  RETURN_VERIFY         // 반납: 닫고 결과 확인
};

struct SlotRuntime {
  FlowState state;
  uint32_t  stateSince;

  bool     servoOpen;
  bool     servoBusy;        // 움직이는 중 (다 움직이면 힘을 뺌)
  uint32_t servoMovedAt;

  bool     rawPresent;       // 디바운스 전 값
  bool     present;          // 디바운스 후 값
  uint32_t rawChangedAt;

  bool     absentTimerRunning;
  uint32_t absentSince;
  bool     presentTimerRunning;
  uint32_t presentSince;

  bool    returnAuto;        // 반납이 1단계(자동) 방식인지
  uint8_t returnAttempts;    // 반납 재개방 횟수
};

struct OutboundMessage { char query[192]; };   // 서버로 보낼 요청

Servo servos[SLOT_COUNT];
SlotRuntime slots[SLOT_COUNT];
QueueHandle_t commandQueue  = nullptr;
QueueHandle_t outboundQueue = nullptr;

struct CommandMessage { char text[64]; };

// millis()가 한 바퀴 돌아도(약 49일) 안전한 경과 시간 비교
bool hasElapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

// ─────────────────────────────────────────────────────────────
// 5. 서보와 리드스위치
// ─────────────────────────────────────────────────────────────

bool readReed(uint8_t i) { return digitalRead(REED_PINS[i]) == REED_ACTIVE_LEVEL; }

// 서보는 붙어 있는 동안 계속 힘을 씁니다. 움직일 때만 붙이고 끝나면 뗍니다.
void moveServo(uint8_t i, bool open) {
  if (!servos[i].attached())
    servos[i].attach(SERVO_PINS[i], SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
  servos[i].write(open ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE);
  slots[i].servoOpen    = open;
  slots[i].servoBusy    = true;
  slots[i].servoMovedAt = millis();
}

// 다 움직인 서보의 힘 빼기 (매 루프에서 확인)
void updateServoPower(uint8_t i, uint32_t now) {
  if (!slots[i].servoBusy) return;
  if (!hasElapsed(now, slots[i].servoMovedAt, SERVO_MOVE_MS)) return;
  slots[i].servoBusy = false;
  if (RELEASE_WHEN_IDLE && servos[i].attached()) servos[i].detach();
}

void setState(uint8_t i, FlowState state, uint32_t now) {
  slots[i].state               = state;
  slots[i].stateSince          = now;
  slots[i].absentTimerRunning  = false;
  slots[i].presentTimerRunning = false;
}

void updateReedSwitches(uint32_t now) {
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    const bool raw = readReed(i);
    if (raw != slots[i].rawPresent) {
      slots[i].rawPresent   = raw;
      slots[i].rawChangedAt = now;
    }
    if (raw != slots[i].present &&
        hasElapsed(now, slots[i].rawChangedAt, REED_DEBOUNCE_MS)) {
      slots[i].present = raw;
    }
  }
}

// ─────────────────────────────────────────────────────────────
// 6. 서버로 보낼 요청 (네트워크 태스크가 실제로 전송)
// ─────────────────────────────────────────────────────────────

void queueRequest(const String& query) {
  if (outboundQueue == nullptr) return;
  OutboundMessage msg{};
  query.toCharArray(msg.query, sizeof(msg.query));
  if (xQueueSend(outboundQueue, &msg, 0) != pdTRUE)
    Serial.println("보낼 요청이 밀렸어요 (큐 가득)");
}

// 대여 취소 — 문은 열렸는데 우산을 안 가져갔을 때 요금이 청구되지 않도록
void requestCancel(uint8_t i) {
  queueRequest(String("?action=cancel&locker_id=") + LOCKER_ID +
               "&slot=" + String(i + 1));
}

// 네 칸의 우산 유무를 서버에 보고 (백엔드가 기대하는 p1~p4 형식)
void requestReport() {
  String q = String("?action=report&locker_id=") + LOCKER_ID;
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    q += "&p" + String(i + 1) + "=" + String(slots[i].present ? 1 : 0);
  queueRequest(q);
}

// ─────────────────────────────────────────────────────────────
// 7. 대여 · 반납 흐름
// ─────────────────────────────────────────────────────────────

void lockSlot(uint8_t i) {
  moveServo(i, false);
  setState(i, FlowState::LOCKED_IDLE, millis());
}

void startRent(uint8_t i) {
  const uint32_t now = millis();

  if (!slots[i].present) {          // 빌려줄 우산이 없음
    Serial.printf("슬롯%u 대여불가 — 우산이 없어요\n", i + 1);
    requestCancel(i);
    return;
  }

  Serial.printf("슬롯%u 열림\n", i + 1);
  moveServo(i, true);
  setState(i, FlowState::RENT_WAIT_REMOVE, now);
}

void startReturn(uint8_t i, bool autoMode) {
  const uint32_t now = millis();
  Serial.printf("슬롯%u 열림\n", i + 1);
  moveServo(i, true);
  slots[i].returnAuto     = autoMode;
  slots[i].returnAttempts = 1;
  setState(i, autoMode ? FlowState::RETURN_WAIT_INSERT
                       : FlowState::RETURN_WAIT_DONE, now);
}

void updateSlotState(uint8_t i, uint32_t now) {
  SlotRuntime& s = slots[i];

  switch (s.state) {
    case FlowState::LOCKED_IDLE:
      break;

    case FlowState::MANUAL_OPEN:
      if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
        Serial.printf("슬롯%u 닫힘 (자동)\n", i + 1);
        lockSlot(i);
      }
      break;

    case FlowState::RENT_WAIT_REMOVE:
      if (!s.present) {                       // 우산을 가져갔다
        if (!s.absentTimerRunning) {
          s.absentTimerRunning = true;
          s.absentSince = now;
        }
        if (hasElapsed(now, s.absentSince, RENT_CLOSE_DELAY_MS)) {
          Serial.printf("%lu초후 닫힘\n", RENT_CLOSE_DELAY_MS / 1000);
          moveServo(i, false);
          setState(i, FlowState::RENT_VERIFY, now);
        }
      } else {
        s.absentTimerRunning = false;         // 다시 넣었으면 카운트 취소
        // 계속 안 가져가면 자동으로 닫고 실패 처리
        if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
          Serial.println("닫힘 (시간 초과)");
          moveServo(i, false);
          setState(i, FlowState::RENT_VERIFY, now);
        }
      }
      break;

    case FlowState::RENT_VERIFY:
      if (!hasElapsed(now, s.stateSince, SERVO_MOVE_MS)) break;
      if (!s.present) {
        Serial.println("대여완료");
      } else {
        Serial.println("대여실패 — 우산이 그대로 있어요");
        requestCancel(i);                     // 요금이 청구되지 않도록
      }
      setState(i, FlowState::LOCKED_IDLE, now);
      requestReport();
      break;

    case FlowState::RETURN_WAIT_INSERT:
      if (s.present) {                        // 우산이 꽂혔다
        if (!s.presentTimerRunning) {
          s.presentTimerRunning = true;
          s.presentSince = now;
        }
        if (hasElapsed(now, s.presentSince, RETURN_PRESENT_HOLD_MS)) {
          Serial.println("닫힘");
          moveServo(i, false);
          setState(i, FlowState::RETURN_VERIFY, now);
        }
      } else {
        s.presentTimerRunning = false;
        if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
          Serial.println("반납실패 — 우산이 안 들어왔어요");
          lockSlot(i);
          requestReport();
        }
      }
      break;

    case FlowState::RETURN_WAIT_DONE:         // 2단계 반납: 닫기 명령 대기
      if (OPEN_FAILSAFE_MS > 0 && hasElapsed(now, s.stateSince, OPEN_FAILSAFE_MS)) {
        Serial.println("반납실패 — 시간이 지났어요");
        lockSlot(i);
        requestReport();
      }
      break;

    case FlowState::RETURN_VERIFY:
      if (!hasElapsed(now, s.stateSince, SERVO_MOVE_MS)) break;
      if (s.present) {
        Serial.println("반납완료");
        setState(i, FlowState::LOCKED_IDLE, now);
        requestReport();
      } else if (s.returnAttempts >= RETURN_RETRY_MAX) {
        Serial.printf("반납실패 — %u번 열어도 우산이 없어요\n", RETURN_RETRY_MAX);
        setState(i, FlowState::LOCKED_IDLE, now);
        requestReport();
      } else {
        s.returnAttempts++;                   // 다시 한 번 기회
        Serial.println("우산이 없어요 — 다시 열게요");
        Serial.printf("슬롯%u 열림\n", i + 1);
        moveServo(i, true);
        setState(i, s.returnAuto ? FlowState::RETURN_WAIT_INSERT
                                 : FlowState::RETURN_WAIT_DONE, now);
      }
      break;
  }
}

// ─────────────────────────────────────────────────────────────
// 8. 명령 해석 (앱·시리얼 공용)
// ─────────────────────────────────────────────────────────────

void printStatus() {
  Serial.println("── 슬롯 상태 ──");
  for (uint8_t i = 0; i < SLOT_COUNT; ++i)
    Serial.printf("  슬롯%u: 우산 %s\n", i + 1, slots[i].present ? "있음" : "없음");
}

bool runSlotCommand(String verb, int slotNumber) {
  verb.trim();
  verb.toUpperCase();
  verb.replace('-', '_');

  if (slotNumber < 1 || slotNumber > SLOT_COUNT) {
    Serial.println("슬롯 번호는 1~4 입니다");
    return true;
  }
  const uint8_t i = static_cast<uint8_t>(slotNumber - 1);

  if (verb == "RENT")                                   { startRent(i); }
  else if (verb == "RETURN")                            { startReturn(i, true); }
  else if (verb == "RETURN_OPEN" || verb == "RETURN_START") { startReturn(i, false); }
  else if (verb == "RETURN_DONE" || verb == "RETURN_COMPLETE" || verb == "RETURN_CLOSE") {
    if (slots[i].state == FlowState::RETURN_WAIT_DONE) {
      Serial.println("닫힘");
      moveServo(i, false);
      setState(i, FlowState::RETURN_VERIFY, millis());
    } else {
      startReturn(i, true);        // 열기 명령을 놓쳤으면 자동 방식으로 복구
    }
  }
  else if (verb == "OPEN" || verb == "UNLOCK") {
    Serial.printf("슬롯%d 열림 (닫으려면 c%d)\n", slotNumber, slotNumber);
    moveServo(i, true);
    setState(i, FlowState::MANUAL_OPEN, millis());
  }
  else if (verb == "CLOSE" || verb == "LOCK") {
    Serial.printf("슬롯%d 닫힘\n", slotNumber);
    lockSlot(i);
  }
  else return false;

  return true;
}

bool isNumber(const String& v) {
  if (v.length() == 0) return false;
  for (size_t i = 0; i < v.length(); ++i) if (!isDigit(v[i])) return false;
  return true;
}

void executeCommand(String command) {
  command.trim();
  command.toUpperCase();
  command.replace('-', '_');
  if (command.length() == 0) return;

  // 시리얼 단축 명령: s / r1 / b1 / o1 / c1
  if (command == "S") { printStatus(); return; }
  if (command.length() == 2 && isDigit(command[1])) {
    const int slot = command[1] - '0';
    switch (command[0]) {
      case 'R': runSlotCommand("RENT", slot);   return;
      case 'B': runSlotCommand("RETURN", slot); return;
      case 'O': runSlotCommand("OPEN", slot);   return;
      case 'C': runSlotCommand("CLOSE", slot);  return;
      default: break;
    }
  }

  // 슬롯 없는 전체 명령
  if (command == "LOCK" || command == "CLOSE") {
    for (uint8_t i = 0; i < SLOT_COUNT; ++i) lockSlot(i);
    Serial.println("전체 닫힘");
    return;
  }

  // "RENT:1" / "RENT:L001:1" / "L001:RENT:1"
  String parts[3];
  uint8_t count = 0;
  int start = 0;
  while (count < 3) {
    const int sep = command.indexOf(':', start);
    if (sep < 0) { parts[count++] = command.substring(start); break; }
    parts[count++] = command.substring(start, sep);
    start = sep + 1;
  }

  String verb, locker, slotText;
  if (count == 2) {
    verb = parts[0]; slotText = parts[1];
  } else if (count == 3) {
    if (parts[0] == LOCKER_ID) { locker = parts[0]; verb = parts[1]; slotText = parts[2]; }
    else                       { verb = parts[0]; locker = parts[1]; slotText = parts[2]; }
  } else {
    Serial.println("모르는 명령: " + command +
                   "  (r1~r4=대여, b1~b4=반납, o1~o4=열기, c1~c4=닫기, s=상태)");
    return;
  }

  locker.trim();
  if (locker.length() > 0 && locker != LOCKER_ID) {
    Serial.println("다른 보관함 명령이라 무시: " + locker);
    return;
  }

  slotText.trim();
  if (!isNumber(slotText) || !runSlotCommand(verb, slotText.toInt()))
    Serial.println("모르는 명령: " + command);
}

// ─────────────────────────────────────────────────────────────
// 9. 네트워크 (별도 코어에서 실행 — 센서 타이밍을 방해하지 않게)
// ─────────────────────────────────────────────────────────────

int httpsGet(const String& url, String& body) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();          // 시제품용. 운영에서는 루트 인증서를 등록하세요.
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) return -1000;
  const int code = http.GET();
  if (code > 0) body = http.getString();
  http.end();
  client.stop();
  return code;
}

// 서버는 "RENT:1" 같은 평문을 돌려줍니다. 나중에 JSON으로 바뀌어도 읽히게 해둡니다.
String decodeCommand(String body) {
  body.trim();
  if (body.length() == 0 || body == "null") return "";

  if (body.startsWith("{")) {
    const char* keys[] = {"\"command\"", "\"cmd\""};
    for (const char* key : keys) {
      const int keyPos = body.indexOf(key);
      if (keyPos < 0) continue;
      const int colon = body.indexOf(':', keyPos + strlen(key));
      if (colon < 0) continue;
      const int q1 = body.indexOf('"', colon + 1);
      if (q1 < 0) continue;
      int q2 = q1 + 1;
      while (q2 < static_cast<int>(body.length())) {
        if (body[q2] == '"' && body[q2 - 1] != '\\') break;
        ++q2;
      }
      if (q2 < static_cast<int>(body.length())) { body = body.substring(q1 + 1, q2); break; }
    }
  } else if (body.length() >= 2 && body[0] == '"' && body[body.length() - 1] == '"') {
    body = body.substring(1, body.length() - 1);
  }

  body.replace("\\\"", "\"");
  body.replace("\\n", "");
  body.replace("\\r", "");
  body.trim();
  return body;
}

void networkTask(void* parameter) {
  (void)parameter;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t lastWiFiTry = millis();
  uint32_t lastPoll    = millis() - COMMAND_POLL_MS;
  uint32_t lastReport  = millis();
  bool wasConnected    = false;

  for (;;) {
    const uint32_t now = millis();
    const bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected != wasConnected) {
      wasConnected = connected;
      Serial.println(connected ? "WiFi 연결됨 — 앱 명령 대기"
                               : "WiFi 끊김 — USB 명령만 동작합니다");
      if (connected) requestReport();
    }

    if (!connected) {
      if (hasElapsed(now, lastWiFiTry, WIFI_RETRY_MS)) {
        lastWiFiTry = now;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // 보낼 요청이 있으면 먼저 처리 (대여 취소가 늦으면 안 되니까)
    OutboundMessage out{};
    if (xQueueReceive(outboundQueue, &out, 0) == pdTRUE) {
      String body;
      const int code = httpsGet(String(SERVER_URL) + out.query, body);
      if (code != HTTP_CODE_OK) Serial.printf("서버 전송 실패 (%d)\n", code);
    }

    // 큐에 자리가 있을 때만 명령을 꺼냅니다 — 받아놓고 잃어버리지 않게
    if (hasElapsed(now, lastPoll, COMMAND_POLL_MS) &&
        uxQueueSpacesAvailable(commandQueue) > 0) {
      lastPoll = now;
      String body;
      const String url = String(SERVER_URL) + "?action=cmd&locker_id=" + LOCKER_ID +
                         "&_=" + String(now);
      const int code = httpsGet(url, body);
      if (code == HTTP_CODE_OK) {
        const String command = decodeCommand(body);
        if (command.length() > 0) {
          CommandMessage msg{};
          command.toCharArray(msg.text, sizeof(msg.text));
          xQueueSend(commandQueue, &msg, 0);
        }
      }
    }

    if (hasElapsed(now, lastReport, REPORT_EVERY_MS)) {
      lastReport = now;
      requestReport();
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ─────────────────────────────────────────────────────────────
// 10. setup / loop
// ─────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== 스마트 우산 보관함 ===");

  commandQueue  = xQueueCreate(8, sizeof(CommandMessage));
  outboundQueue = xQueueCreate(12, sizeof(OutboundMessage));
  if (commandQueue == nullptr || outboundQueue == nullptr) {
    Serial.println("메모리 부족 — 다시 켜주세요");
    while (true) delay(1000);
  }

  // 네 칸을 한 개씩 차례로 잠급니다 — 동시에 움직이면 전류가 확 튀어
  // 약한 전원에서는 보드가 리셋됩니다.
  Serial.println("네 칸을 차례로 잠급니다...");
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    pinMode(REED_PINS[i], INPUT_PULLUP);

    const bool present = readReed(i);
    slots[i] = SlotRuntime{};
    slots[i].state        = FlowState::LOCKED_IDLE;
    slots[i].rawPresent   = present;
    slots[i].present      = present;
    slots[i].rawChangedAt = millis();
    slots[i].stateSince   = millis();

    servos[i].setPeriodHertz(50);
    Serial.printf("슬롯%u 잠금\n", i + 1);
    moveServo(i, false);
    delay(SERVO_MOVE_MS);                       // 다 움직일 때까지 기다렸다가
    slots[i].servoBusy = false;
    if (RELEASE_WHEN_IDLE) servos[i].detach();  // 힘 빼고 다음 칸으로
  }

  printStatus();
  Serial.println("시리얼 명령");
  Serial.println("  r1~r4 = 대여   b1~b4 = 반납   s = 슬롯 상태");
  Serial.println("  o1~o4 = 그냥 열기   c1~c4 = 그냥 닫기  (서보·전원 점검용)");

  if (xTaskCreatePinnedToCore(networkTask, "umbrella-net", 8192,
                              nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("네트워크 시작 실패 — USB 명령만 동작합니다");
  }
}

void loop() {
  const uint32_t now = millis();

  // ① USB(시리얼) 명령
  if (Serial.available()) executeCommand(Serial.readStringUntil('\n'));

  // ② 앱에서 온 명령
  CommandMessage msg{};
  while (xQueueReceive(commandQueue, &msg, 0) == pdTRUE)
    executeCommand(String(msg.text));

  // ③ 센서 읽기 · 상태 진행 · 서보 힘 빼기
  updateReedSwitches(now);
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    updateSlotState(i, now);
    updateServoPower(i, now);
  }

  delay(5);
}
