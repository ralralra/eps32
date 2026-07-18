// ESP32-CAM 단순 움직임 감지 (프레임 차분)
// 보드: AI Thinker ESP32-CAM, PSRAM Enabled, Huge APP

#include "esp_camera.h"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN      4

// 튜닝 값
const int   DIFF_THRESHOLD = 25;     // 픽셀당 밝기 차이
const float MOTION_RATIO   = 0.05f;  // 전체 픽셀 중 5% 이상 바뀌면 움직임
const unsigned long COOLDOWN_MS = 2000;

// QQVGA 160x120 grayscale → 19200 픽셀
const int W = 160, H = 120, N = W * H;
uint8_t prev[N];
bool havePrev = false;
unsigned long lastTrigger = 0;

bool initCamera() {
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0=Y2_GPIO_NUM; cfg.pin_d1=Y3_GPIO_NUM; cfg.pin_d2=Y4_GPIO_NUM; cfg.pin_d3=Y5_GPIO_NUM;
  cfg.pin_d4=Y6_GPIO_NUM; cfg.pin_d5=Y7_GPIO_NUM; cfg.pin_d6=Y8_GPIO_NUM; cfg.pin_d7=Y9_GPIO_NUM;
  cfg.pin_xclk=XCLK_GPIO_NUM; cfg.pin_pclk=PCLK_GPIO_NUM;
  cfg.pin_vsync=VSYNC_GPIO_NUM; cfg.pin_href=HREF_GPIO_NUM;
  cfg.pin_sccb_sda=SIOD_GPIO_NUM; cfg.pin_sccb_scl=SIOC_GPIO_NUM;
  cfg.pin_pwdn=PWDN_GPIO_NUM; cfg.pin_reset=RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_GRAYSCALE;   // 회색조 1byte/px
  cfg.frame_size   = FRAMESIZE_QQVGA;       // 160x120
  cfg.fb_count     = 1;
  cfg.grab_mode    = CAMERA_GRAB_LATEST;
  cfg.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  return esp_camera_init(&cfg) == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-CAM Motion Detection ===");
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  if (!initCamera()) { Serial.println("Camera init fail"); while (true) delay(1000); }
}

void loop() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->len < (size_t)N) {
    if (fb) esp_camera_fb_return(fb);
    delay(50);
    return;
  }

  if (!havePrev) {
    memcpy(prev, fb->buf, N);
    havePrev = true;
    esp_camera_fb_return(fb);
    return;
  }

  unsigned long diffPixels = 0;
  for (int i = 0; i < N; i++) {
    int d = (int)fb->buf[i] - (int)prev[i];
    if (d < 0) d = -d;
    if (d > DIFF_THRESHOLD) diffPixels++;
  }
  memcpy(prev, fb->buf, N);
  esp_camera_fb_return(fb);

  float ratio = (float)diffPixels / (float)N;
  unsigned long now = millis();
  bool motion = ratio >= MOTION_RATIO && (now - lastTrigger) > COOLDOWN_MS;

  if (motion) {
    lastTrigger = now;
    Serial.printf(">> MOTION! changed=%lu (%.2f%%)\n", diffPixels, ratio * 100.0f);
    digitalWrite(FLASH_LED_PIN, HIGH); delay(80);
    digitalWrite(FLASH_LED_PIN, LOW);
    // TODO: 여기서 esp_camera_deinit() 후 JPEG 모드로 재초기화하여 SD 저장 등
  } else {
    Serial.printf("idle  changed=%lu (%.2f%%)\n", diffPixels, ratio * 100.0f);
  }

  delay(150);  // 너무 빨리 돌면 잡음만 증가
}
