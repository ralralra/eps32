// ESP32-CAM 최소 MJPEG 스트리밍 서버
// 보드: AI Thinker ESP32-CAM, PSRAM Enabled, Huge APP
//
// 접속: http://<IP>/        (간단 페이지)
//      http://<IP>/stream  (MJPEG 스트림 단독)

#include <WiFi.h>
#include <esp_http_server.h>
#include "esp_camera.h"

// 4단계의 camera_pins.h 와 동일 — 이 폴더에도 복사해 두면 편함
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

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

httpd_handle_t server = NULL;

#define BOUNDARY "frame"
static const char* STREAM_CT  = "multipart/x-mixed-replace;boundary=" BOUNDARY;
static const char* STREAM_HDR = "\r\n--" BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t indexHandler(httpd_req_t* req) {
  const char* html =
    "<!doctype html><html><head><meta charset=utf-8>"
    "<title>ESP32-CAM</title></head><body style='margin:0;background:#111'>"
    "<img src='/stream' style='width:100%;display:block'/></body></html>";
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t streamHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, STREAM_CT);
  char hdr[64];
  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { Serial.println("fb_get fail"); return ESP_FAIL; }

    size_t hlen = snprintf(hdr, sizeof(hdr), STREAM_HDR, fb->len);
    if (httpd_resp_send_chunk(req, hdr, hlen) != ESP_OK ||
        httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len) != ESP_OK) {
      esp_camera_fb_return(fb);
      break;
    }
    esp_camera_fb_return(fb);
  }
  return ESP_OK;
}

void startServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  httpd_uri_t idx    = { .uri="/",       .method=HTTP_GET, .handler=indexHandler,  .user_ctx=NULL };
  httpd_uri_t strm   = { .uri="/stream", .method=HTTP_GET, .handler=streamHandler, .user_ctx=NULL };
  if (httpd_start(&server, &cfg) == ESP_OK) {
    httpd_register_uri_handler(server, &idx);
    httpd_register_uri_handler(server, &strm);
  }
}

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
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.grab_mode    = CAMERA_GRAB_LATEST;
  cfg.frame_size   = FRAMESIZE_VGA;
  cfg.jpeg_quality = 12;
  cfg.fb_count     = psramFound() ? 2 : 1;
  cfg.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  return esp_camera_init(&cfg) == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-CAM Simple Stream ===");

  if (!initCamera()) { Serial.println("Camera init fail"); while (true) delay(1000); }

  WiFi.begin(ssid, password);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print('.'); }
  Serial.printf("\nOpen: http://%s/\n", WiFi.localIP().toString().c_str());

  startServer();
}

void loop() { delay(1000); }
