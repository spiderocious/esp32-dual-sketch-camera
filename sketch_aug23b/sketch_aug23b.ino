#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// Replace with your WiFi credentials
const char *ssid = "NETWORK";
const char *password = "PASSWORD";

WebServer server(80);

// Camera pins for AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void setup()
{
  Serial.begin(115200);

  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
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
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESPC IP: ");
  Serial.println(WiFi.localIP());

  server.on("/status", []()
            { 
    String status = "ESP32-CAM Status:\n";
    status += "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    status += "WiFi RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    status += "IP: " + WiFi.localIP().toString() + "\n";
    server.send(200, "text/plain", status); });

  server.on("/ready", []()
            {
  Serial.println("Ready check requested");
  
  // Check if camera is initialized
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    server.send(503, "text/plain", "Camera not initialized");
    Serial.println("Camera not ready - not initialized");
    return;
  }
  
  // Clear buffers to ensure freshness
  bool bufferCleared = false;
  for (int i = 0; i < 2; i++) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb);
      bufferCleared = true;
    }
    delay(5);
  }
  
  // Test capture to verify camera is working
  camera_fb_t * testFb = esp_camera_fb_get();
  if (!testFb) {
    server.send(503, "text/plain", "Camera not responding");
    Serial.println("Camera not ready - test capture failed");
    return;
  }
  
  // Return test frame and confirm ready
  esp_camera_fb_return(testFb);
  
  String response = "Camera ready\n";
  response += "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
  response += "Buffers cleared: " + String(bufferCleared ? "Yes" : "No") + "\n";
  
  server.send(200, "text/plain", response);
  Serial.println("Camera is ready for capture"); });

  // Simple route to capture image
  server.on("/capture", []()
            {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "text/plain", "Camera failed");
      return;
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    Serial.println("Image captured and sent"); });

  server.begin();
  Serial.println("Camera server started");
}

void loop()
{
  server.handleClient();
}