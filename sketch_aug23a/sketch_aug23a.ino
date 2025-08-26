#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "MTN-5G-4ACC15";
const char* password = "JESU1213";


// ESPC IP address (check serial monitor of ESPC for actual IP)
const char* espc_ip = "http://192.168.0.134/capture"; // Update this after ESPC connects

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.print("Started...");
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESPM IP: ");
  Serial.println(WiFi.localIP());
  
  // Main web page
  server.on("/", [](){
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Camera System</title>
    <style>
        body { font-family: Arial; text-align: center; margin: 50px; }
        button { 
            background: #4CAF50; 
            color: white; 
            border: none; 
            padding: 20px 40px; 
            font-size: 18px; 
            border-radius: 5px; 
            cursor: pointer; 
        }
        button:hover { background: #45a049; }
        #imageContainer { margin-top: 30px; }
        img { max-width: 80%; border: 2px solid #ddd; }
    </style>
</head>
<body>
    <h1>ESP32 Camera System</h1>
    <button onclick=captureImage()>Capture Photo</button>
    
    <div id="imageContainer">
        <p id="status">Click "Capture Photo" to take a picture</p>
        <div id="imageDiv"></div>
    </div>

    <script>
        function captureImage() {
            document.getElementById('status').innerHTML = 'Capturing image...';
            document.getElementById('imageDiv').innerHTML = '';
            
            fetch('/capture')
            .then(response => {
                if (response.ok) {
                    return response.blob();
                }
                throw new Error('Capture failed');
            })
            .then(blob => {
                const img = document.createElement('img');
                img.src = URL.createObjectURL(blob);
                document.getElementById('imageDiv').appendChild(img);
                document.getElementById('status').innerHTML = 'Image captured successfully!';
            })
            .catch(error => {
                document.getElementById('status').innerHTML = 'Error: ' + error.message;
            });
        }
    </script>
</body>
</html>
)";
    server.send(200, "text/html", html);
  });
  
  // Capture endpoint - gets image from ESPC
  server.on("/capture", [](){
    HTTPClient http;
    http.begin(String(espc_ip));
    
    int httpCode = http.GET();
    if (httpCode == 200) {
      String imageData = http.getString();
      server.send(200, "image/jpeg", imageData);
      Serial.println("Image forwarded from ESPC");
    } else {
      server.send(500, "text/plain", "Failed to get image from camera");
      Serial.println("Failed to connect to ESPC");
    }
    http.end();
  });
  
  server.begin();
  Serial.println("Main web server started");
  Serial.println("Open browser and go to: http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}