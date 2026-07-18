// ESP32-CAM 첫 업로드 — Blink + Serial Hello
// 보드: AI Thinker ESP32-CAM, PSRAM Enabled

#define LED_RED   33  // 보드 뒷면 빨간 LED (active-low: LOW=ON)
#define LED_FLASH  4  // 앞면 흰색 플래시 LED (active-high: HIGH=ON)

unsigned long counter = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== ESP32-CAM Blink Test ===");
  Serial.printf("Chip: %s rev %d, %d cores @ %d MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("Flash: %d MB, PSRAM: %d bytes\n",
                ESP.getFlashChipSize() / (1024 * 1024),
                ESP.getPsramSize());

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_FLASH, OUTPUT);
  digitalWrite(LED_FLASH, LOW);  // 너무 밝으니 끈 채로 시작
}

void loop() {
  digitalWrite(LED_RED, LOW);   // ON
  delay(500);
  digitalWrite(LED_RED, HIGH);  // OFF
  delay(500);

  counter++;
  Serial.printf("tick %lu  uptime=%lus  free heap=%u\n",
                counter, millis() / 1000, ESP.getFreeHeap());
}
