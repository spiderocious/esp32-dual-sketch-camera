#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Replace with your WiFi credentials
const char *ssid = "MTN-5G-4ACC15";
const char *password = "JESU1213";

// Upload server configuration
const char *uploadHost = "storex-production-f286.up.railway.app";
const char *uploadPath = "/api/v1/public/file/upload";
const char *bearerToken = "Bearer STOREXXXBUCKETPUBLICXXSTOREX00169B28A885175607330728785276583ED7542AC21756073307287";
const int uploadPort = 443;

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
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brown-out detector
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_CIF;
  config.jpeg_quality = 20;
  config.fb_count = 1;
  Serial.println("No PSRAM - using standard settings");

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed: 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Open browser and go to: http://" + WiFi.localIP().toString() + "/capture-and-upload");

  server.on("/status", []()
            { 
    String status = "ESP32-CAM Status:\n";
    status += "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    status += "WiFi RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    status += "IP: " + WiFi.localIP().toString() + "\n";
    status += "PSRAM: " + String(psramFound() ? "Found" : "Not found") + "\n";
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

  // Optimized capture and upload endpoint
  server.on("/capture-and-upload", []()
            {
    // Capture image
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "application/json", "{\"error\":\"Camera capture failed\"}");
      Serial.println("Camera capture failed");
      return;
    }
    
    Serial.printf("Image captured: %d bytes\n", fb->len);
    
    // Upload using optimized chunked method
    String result = uploadImageChunked(fb);
    esp_camera_fb_return(fb);
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", result);
      Serial.println("Upload successful"); });

  server.begin();
  Serial.println("Camera server started");
  Serial.printf("Free heap after setup: %d bytes\n", ESP.getFreeHeap());
}

String uploadImageChunked(camera_fb_t *fb)
{
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation

  Serial.println("Connecting to upload server...");
  if (!client.connect(uploadHost, uploadPort))
  {
    return "Connection failed";
  }

  Serial.println("Connected! Uploading image...");

  // Create multipart form data
  String boundary = "----formdata-esp32-" + String(millis());
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"esp32cam_" + String(millis()) + ".jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t imageLen = fb->len;
  uint32_t extraLen = head.length() + tail.length();
  uint32_t totalLen = imageLen + extraLen;

  // Send HTTP headers
  client.println("POST " + String(uploadPath) + " HTTP/1.1");
  client.println("Host: " + String(uploadHost));
  client.println("Authorization: " + String(bearerToken));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println();

  // Send multipart head
  client.print(head);

  // Send image data in chunks
  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n += 1024)
  {
    if (n + 1024 < fbLen)
    {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    }
    else if (fbLen % 1024 > 0)
    {
      size_t remainder = fbLen % 1024;
      client.write(fbBuf, remainder);
    }
  }

  // Send multipart tail
  client.print(tail);

  // Read response with timeout
  int timeoutTimer = 15000; // 15 seconds
  long startTimer = millis();
  String getAll;
  String getBody;
  boolean state = false;

  while ((startTimer + timeoutTimer) > millis())
  {
    Serial.print(".");
    delay(100);
    while (client.available())
    {
      char c = client.read();
      if (c == '\n')
      {
        if (getAll.length() == 0)
        {
          state = true;
        }
        getAll = "";
      }
      else if (c != '\r')
      {
        getAll += String(c);
      }
      if (state == true)
      {
        getBody += String(c);
      }
      startTimer = millis();
    }
    if (getBody.length() > 0)
    {
      break;
    }
  }

  Serial.println();
  client.stop();

  if (getBody.length() == 0)
  {
    return "No response from server";
  }

  // Parse JSON response manually
  Serial.println("Server response: " + getBody);

  if (getBody.indexOf("\"success\":true") > -1)
  {
    // Extract download URL
    int urlStart = getBody.indexOf("\"downloadUrl\":\"") + 15;
    int urlEnd = getBody.indexOf("\"", urlStart);
    String downloadUrl = getBody.substring(urlStart, urlEnd);

    if (downloadUrl.length() > 0)
    {
      return downloadUrl;
    }
  }

  return "Failed to parse server response";
}

void loop()
{
  server.handleClient();
  yield(); // Allow other tasks to run
}