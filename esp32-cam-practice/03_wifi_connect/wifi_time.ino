// ESP32-CAM Wi-Fi 접속 + NTP 시각
// 보드: AI Thinker ESP32-CAM, PSRAM Enabled
//
// 시리얼 모니터(115200)에서 IP와 한국 시간이 1초마다 출력되면 성공.

#include <WiFi.h>
#include <time.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// 한국: UTC+9, DST 없음
const long  gmtOffset_sec      = 9 * 3600;
const int   daylightOffset_sec = 0;
const char* ntpServer1 = "kr.pool.ntp.org";
const char* ntpServer2 = "time.google.com";

void connectWiFi() {
  Serial.printf("\nConnecting to \"%s\" ", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
    if (millis() - start > 20000) {
      Serial.println(" TIMEOUT, restarting...");
      ESP.restart();
    }
  }
  Serial.println(" OK");
  Serial.printf("IP:    %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("RSSI:  %d dBm\n", WiFi.RSSI());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-CAM Wi-Fi + NTP ===");

  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  Serial.println("Waiting for NTP sync...");
  struct tm t;
  while (!getLocalTime(&t, 5000)) {
    Serial.println("  NTP fail, retry");
  }
  Serial.println("NTP synced.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost, reconnecting...");
    connectWiFi();
  }
  struct tm t;
  if (getLocalTime(&t)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    Serial.printf("[%s]  RSSI=%d  heap=%u\n",
                  buf, WiFi.RSSI(), ESP.getFreeHeap());
  }
  delay(1000);
}
