/*
  3단계 — 자동 스마트팜 (키트 예제 12_SmartFarm_KIT의 D1 R32 버전, 인터넷 없이!)
  강낭콩 발아 기준으로 온도·습도·토양습도를 읽어 팬과 LED를 스스로 제어해요.

  ── 자동 규칙 (기준값은 우리 화분에 맞게 조절!) ──
  온도 26℃ 초과      → 팬 ON (더위 식히기)
  온도 21℃ 미만      → 네오픽셀 따뜻한 빛 (보온)
  습도 80% 초과      → 팬 ON (곰팡이 방지 환기)
  토양습도 70% 초과  → 네오픽셀 따뜻한 빛 (과습 말리기 — 키트 자료 방식)
  토양습도 30% 미만  → 시리얼에 "물 주세요!" (키트엔 펌프가 없어서 사람이 급수!)

  ── 배선 (D1 R32 변환 완료) ──
  토양수분 → A2 자리(GPIO35) / DHT11 → D2 자리(GPIO26)
  팬 → D4·D5·D6·D7 자리(17·16·27·14, 모터드라이버 경유) / 네오픽셀 → D9 자리(GPIO13)
*/

#include <DHT.h>
#include <Adafruit_NeoPixel.h>

#define SOIL   35
#define DHTPIN 26
#define AA 16
#define AB 17
#define BA 27
#define BB 14
#define LEDPIN 13
#define NUMLED 4

#define SOIL_MAX 4095   // 01_soil에서 찾은 우리 화분의 최댓값으로!

// ── 강낭콩 발아 기준 (생육정보: 21.6~25.8℃ · 습도 50%) ──
#define TEMP_HIGH 26    // 이보다 더우면 팬
#define TEMP_LOW  21    // 이보다 추우면 보온등
#define HUMI_HIGH 80    // 이보다 습하면 환기
#define SOIL_WET  70    // 이보다 젖었으면 말리기
#define SOIL_DRY  30    // 이보다 마르면 급수 알림

DHT dht(DHTPIN, DHT11);
Adafruit_NeoPixel led(NUMLED, LEDPIN, NEO_GRB + NEO_KHZ800);

void fan(bool on) {
  digitalWrite(AA, on ? HIGH : LOW); digitalWrite(AB, LOW);
  digitalWrite(BA, on ? HIGH : LOW); digitalWrite(BB, LOW);
}

void warmLight(bool on) {
  for (int i = 0; i < NUMLED; i++)
    led.setPixelColor(i, on ? led.Color(255, 150, 60) : 0);
  led.show();
}

void setup() {
  Serial.begin(115200);
  pinMode(AA, OUTPUT); pinMode(AB, OUTPUT);
  pinMode(BA, OUTPUT); pinMode(BB, OUTPUT);
  dht.begin();
  led.begin();
  led.setBrightness(150);
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soilPct = map(analogRead(SOIL), 0, SOIL_MAX, 0, 100);
  Serial.printf("T %.1f℃  H %.0f%%  Soil %d%%  → ", t, h, soilPct);

  bool needFan  = (t > TEMP_HIGH) || (h > HUMI_HIGH);
  bool needWarm = (t < TEMP_LOW) || (soilPct > SOIL_WET);

  fan(needFan);
  warmLight(needWarm);

  if (needFan)  Serial.print("팬ON ");
  if (needWarm) Serial.print("보온등ON ");
  if (soilPct < SOIL_DRY) Serial.print("💧물 주세요! ");
  Serial.println();

  delay(2000);
}
