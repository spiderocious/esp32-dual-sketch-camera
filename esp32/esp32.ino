#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h> // Keypad by mark stanley
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFi.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Connect to these pins (adjust if needed)
byte rowPins[ROWS] = {19, 18, 5, 17}; // R1, R2, R3, R4
byte colPins[COLS] = {16, 4, 2, 15};  // C1, C2, C3, C4

// Create the Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Special keys
#define RESTART_KEY '*'
#define RESTART_ESP_KEY '7'

#define WHITE_LED 13
#define BLUE_LED 27
#define RED_LED 12

#define BUILTIN_LED 2


const char *espc_ip = "http://192.168.0.134/capture-and-upload";
const char *ready_cam = "http://192.168.0.134/ready";

const char *ssid = "MTN-5G-4ACC15";
const char *password = "JESU1213";

const char *analysis_submission = "https://dundie-backend-production.up.railway.app/submit-analysis/short?imageUrl=";
const char *result_api = "https://dundie-backend-production.up.railway.app/analysis/short/";

void setup()
{
  Serial.begin(115200);
  delay(1000);
  blinkLED(2, 50);

  // Initialize LCD
  Wire.begin();
  delay(100);

  lcd.begin(16, 2);
  delay(100);

  lcd.clear();
  delay(100);

  lcd.backlight();
  delay(100);

  lcd.display();
  lcd.noCursor();
  lcd.noBlink();

  displayText("Starting up...");

  displayText(String("Connecting to ") + ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  displayText("Wifi Connected");

  Serial.println("\nWiFi connected!");
  Serial.print("ESPM IP: ");
  Serial.println(WiFi.localIP());

  Serial.println("System started - Keypad library version");
  displayText("WELCOME");
  delay(2500);
  // Main program flow
  runMainProgram();
}

void loop()
{
  // Check for special keys continuously
  char key = keypad.getKey();

  if (key)
  {
    Serial.print("Key pressed: ");
    Serial.println(key);
    if (key == RESTART_KEY)
    {
      runMainProgram(); // Go back to main menu
    }

    if (key == RESTART_ESP_KEY)
    {
      showLoading("Restarting...");
      delay(1000);
      ESP.restart(); // Restart ESP32
    }
  }

  delay(10); // Small delay
}

void runMainProgram()
{
  // Show welcome message
  // Show main menu and wait for input
  while (true)
  {
    displayText("A: QUICK TEST\nB: CHECK RESULT");

    char choice = waitForKeypress("AB"); // Only accept 1 or 2

    if (choice == 'A')
    {
      runTest();
      // Add your quick test code here
      break;
    }
    else if (choice == 'B')
    {
      displayText("Enter Test Code");
      String testCode = getTextInput(10);
      getAnalysisResult(testCode);
      break;
    }
  }
}

void runTest(){
    showLoading("Setting Up...");
    String checkEspCam = fetch(ready_cam);
    if(checkEspCam == "ERROR"){
      displayText("Camera unresponsive");
    }
    else {
      displayText("Please insert your nail for picture \n press A to start!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Insert your nail");
      lcd.setCursor(0,1);
      lcd.print(" Press A to start!")
      char start = waitForKeypress("A");
      if(start == 'A'){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Capturing...");
        lcd.setCursor(0,1);
        lcd.print(" Please wait!")
        String checkEspCam = fetch(ready_cam);
        lcd.clear();
        if(checkEspCam == "ERROR"){
          displayText("Camera unresponsive");
        }
        else {
          delay(2000);
          String imageUrl = fetch(espc_ip);
          if(imageUrl == "ERROR"){
            displayText("Image Capture Failed");
          }
          else {
            String code = fetch(analysis_submission + imageUrl);
            if(code == "ERROR"){
              displayText("Submit Failed");
            }
            else {
              getAnalysisResult(code);
            }
          }

        }
        

      }
    }
}

void getAnalysisResult(String code){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Getting result, please wait.");
  lcd.setCursor(0, 1);
  lcd.print(code);

  String result = getResult(code);
  lcd.clear();
  if(result == "ERROR"){
    displayText("Could not get your result");
  }else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Result READY");
    lcd.setCursor(0, 1);
    lcd.print(" " + code);
    delay(2000);
    displayText(result);
  }
}

// Wait for specific keypress (blocking)
char waitForKeypress(String validKeys)
{
  while (true)
  {
    char key = keypad.getKey();

    if (key)
    {
      Serial.print("Key pressed: ");
      Serial.println(key);

      // Check for special keys first
      if (key == RESTART_KEY)
      {
        runMainProgram();
      }
      // Check if key is in valid keys
      else if (validKeys.indexOf(key) != -1)
      {
        return key;
      }
    }

    delay(10);
  }
}

// Get text input (improved with library)
String getTextInput(int maxLength)
{
  String input = "";
  while (true)
  {
    char key = keypad.getKey();

    if (key)
    {
      Serial.print("Input key: ");
      Serial.println(key);

      if (key == RESTART_KEY)
      {
        runMainProgram();
      }
      else if (input.length() < maxLength)
      {
        // Accept numbers
        input += key;
      }

      // Changed from 'else if' to 'if' - this is the key fix!
      if (input.length() == maxLength)
      {
        return input;
      }

      // Update display
      lcd.setCursor(0, 1);
      lcd.print(input + "_"); // Show cursor
    }

    delay(10);
  }
}

// Test all keys function
void testAllKeys()
{
  displayText("Key Test Mode\nPress any key");

  while (true)
  {
    char key = keypad.getKey();

    if (key)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Key pressed:");
      lcd.setCursor(0, 1);
      lcd.print("'" + String(key) + "'");

      Serial.print("Test - Key pressed: ");
      Serial.println(key);
    }

    delay(10);
  }
}

String fetch(String url) {
 HTTPClient http;
 http.begin(url);
 http.setTimeout(120000);
 Serial.println("making api call");
 Serial.println(url);
 int httpCode = http.GET();
 String response = "";
  Serial.println(httpCode);
 if (httpCode == 200) {
   response = http.getString();
 } else {
   response = "ERROR";
 }
 
 http.end();
 Serial.println(response);
 return response;
}

String getResult(String code) {
 String url =  result_api + code;
 String result = "";
 
 do {
   result = fetch(url);
   if (result != "0" && result != "ERROR") {
     break;
   }
   delay(2000); // Wait 2 seconds before next poll
 } while (true);
 
 return result;
}

// Display function (same as before)
void displayText(String text)
{
  lcd.clear();

  int newlineIndex = text.indexOf('\n');

  if (newlineIndex != -1)
  {
    String line1 = text.substring(0, newlineIndex);
    String line2 = text.substring(newlineIndex + 1);

    if (line1.length() <= 16)
    {
      lcd.setCursor(0, 0);
      lcd.print(line1);
    }
    else
    {
      scrollText(line1, 0);
      return;
    }

    if (line2.length() <= 16)
    {
      lcd.setCursor(0, 1);
      lcd.print(line2);
    }
    else
    {
      scrollText(line2, 1);
    }
  }
  else
  {
    if (text.length() <= 16)
    {
      int pos = (16 - text.length()) / 2;
      lcd.setCursor(pos, 0);
      lcd.print(text);
    }
    else
    {
      scrollText(text, 0);
    }
  }
}

void scrollText(String text, int line)
{
  text = "    " + text + "    ";

  for (int pos = 0; pos <= text.length() - 16; pos++)
  {
    lcd.setCursor(0, line);
    lcd.print(text.substring(pos, pos + 16));
    delay(300);
  }
}


void showLoading(String text)
{
  int maxDots = 8;
  int speed = 300; // milliseconds between dot changes

  for (int cycle = 0; cycle < 3; cycle++)
  { // Run 3 complete cycles
    // Increasing dots
    for (int dots = 1; dots <= maxDots; dots++)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(text);

      // Add dots on second line
      lcd.setCursor(0, 1);
      for (int i = 0; i < dots; i++)
      {
        lcd.print(".");
      }

      delay(speed);
    }

    // Decreasing dots
    for (int dots = maxDots - 1; dots >= 1; dots--)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(text);

      // Add dots on second line
      lcd.setCursor(0, 1);
      for (int i = 0; i < dots; i++)
      {
        lcd.print(".");
      }

      delay(speed);
    }
  }
}

void blinkLED(int pin, int times) {
 for(int i = 0; i < times; i++) {
   digitalWrite(pin, HIGH);
   delay(200);
   digitalWrite(pin, LOW);
   delay(200);
 }
}
