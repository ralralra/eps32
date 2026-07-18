// ESP32-CAM 카메라 초기화 + JPEG 한 장 캡처
// 보드: AI Thinker ESP32-CAM, PSRAM Enabled, Huge APP

#include "esp_camera.h"
#include "camera_pins.h"

bool initCamera() {
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;
  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;
  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;
  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;
  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk  = XCLK_GPIO_NUM;
  cfg.pin_pclk  = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM;
  cfg.pin_href  = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn  = PWDN_GPIO_NUM;
  cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.grab_mode    = CAMERA_GRAB_LATEST;

  // PSRAM 있으면 고해상도, 없으면 낮춰서
  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_VGA;   // 640x480 부터 시작
    cfg.jpeg_quality = 12;              // 0(고화질) ~ 63(저화질)
    cfg.fb_count     = 2;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    cfg.frame_size   = FRAMESIZE_QVGA;  // 320x240
    cfg.jpeg_quality = 15;
    cfg.fb_count     = 1;
    cfg.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  // 센서 부가 설정 (선택)
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);   // -2 ~ 2
    s->set_saturation(s, 0);   // -2 ~ 2
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-CAM Basic Capture ===");
  Serial.printf("PSRAM: %s (%d bytes)\n",
                psramFound() ? "yes" : "no", ESP.getPsramSize());

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!initCamera()) {
    Serial.println("Camera failed. Halt.");
    while (true) delay(1000);
  }
  Serial.println("Camera ready.");
}

void loop() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("fb_get failed");
    delay(1000);
    return;
  }
  Serial.printf("JPEG captured: %ux%u, %u bytes, format=%d\n",
                fb->width, fb->height, fb->len, fb->format);
  esp_camera_fb_return(fb);

  delay(5000);
}
