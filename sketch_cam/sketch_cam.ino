#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

/* =================== WLAN =================== */
const char* ssid = "PippiLANstrumpf";
const char* password = "*Ju04Ja23Le06*";

/* ======= Feste IP ======= */
IPAddress local_IP(192, 168, 178, 50);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 168, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

/* =================== LED =================== */
#define FLASH_LED_PIN 4   // Taschenlampen-LED

/* =================== Kamera Pins (AI Thinker) =================== */
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

httpd_handle_t stream_httpd = NULL;

/* ======= MJPEG Stream Handler ======= */
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  char part_buf[64];

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Kamera Fehler");
      return ESP_FAIL;
    }

    size_t hlen = snprintf(part_buf, 64,
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      fb->len);

    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res != ESP_OK) break;

    res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    if (res != ESP_OK) break;

    res = httpd_resp_send_chunk(req, "\r\n", 2);
    if (res != ESP_OK) break;

    esp_camera_fb_return(fb);
    fb = NULL;
  }

  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_start(&stream_httpd, &config);

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_register_uri_handler(stream_httpd, &stream_uri);
}

void setup() {
  Serial.begin(115200);

  /* ===== LED AUS ===== */
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  /* ===== Kamera Setup ===== */
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  /* 🔥 Performance-Tuning für bessere Qualität */
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10;          // bessere Qualität
  config.fb_count = 2;               // ausreichend für flüssiges Streaming

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Kamera Start fehlgeschlagen");
    return;
  }

  /* Optional: Helligkeit/ Kontrast leicht erhöhen */
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1);  // Helligkeit +1
  s->set_contrast(s, 1);    // Kontrast +1
  s->set_saturation(s, 1);  // Sättigung +1

  /* ===== WLAN mit fester IP ===== */
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);

  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nVerbunden!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  startCameraServer();
}

void loop() {
}
