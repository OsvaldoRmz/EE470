#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
// ===== USER CONFIG =====
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// Hostinger JSON file (for reading RGB value)
const char* STATUS_URL = "https://osvaldoramirezhernandez.com/Led/results.txt"; // returns {"led":"ON","rgb":123}

// Google Sheets Apps Script URL
const char* GAS_URL = "";

// ===== HARDWARE CONFIG =====
const int LED_PIN = D5;      // regular LED
const int R_PIN   = D6;      // RGB Red channel
const int SW1_PIN = D7;      // button 1 -> send status
const int SW2_PIN = D1;      // button 2 -> send RGB

/********** BEHAVIOR CONFIG **********/
const bool LED_ACTIVE_HIGH = true;  // set false if LED is wired to 3V3 (active LOW)
const bool COMMON_ANODE    = false; // true if RGB LED is common-anode (invert PWM)

const unsigned long DEBOUNCE_MS = 150;

/********** STATE **********/
bool ledState = false;               // OFF initially
unsigned long sw1Last = 0, sw2Last = 0;

/********** FORWARD DECLARATIONS **********/
bool fetchStateFromServer(String &ledOut, int &rgbOut);
bool sendToGoogle(const String& params);
void applyLed();
void setRed(uint8_t val);

/******************* SETUP *******************/
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  ledState = false;
  applyLed();

  pinMode(R_PIN, OUTPUT);
  analogWriteRange(255); // 8-bit PWM

  pinMode(SW1_PIN, INPUT_PULLUP);  // button to GND
  pinMode(SW2_PIN, INPUT_PULLUP);  // button to GND

  // Connect WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi OK");

  // Optional: sync once at boot so LED/RGB matches server immediately
  String s; int rv;
  if (fetchStateFromServer(s, rv)) {
    ledState = (s == "ON");
    applyLed();
    setRed((uint8_t)rv);
    Serial.printf("status:%s rgb:%d\n", s.c_str(), rv);
  }
}

/******************* LOOP *******************/
void loop() {
  // ---- SW1: fetch & apply STATUS (no local toggle), then log to Sheets ----
  if (digitalRead(SW1_PIN) == LOW && millis() - sw1Last > DEBOUNCE_MS) {
    sw1Last = millis();
    // Wait for stable press
    delay(30);
    if (digitalRead(SW1_PIN) == LOW) {
      String statusStr; int rgbVal;
      if (fetchStateFromServer(statusStr, rgbVal)) {
        ledState = (statusStr == "ON");
        applyLed();
        sendToGoogle(String("status=") + statusStr);
        Serial.printf("Applied STATUS: %s\n", statusStr.c_str());
      } else {
        Serial.println("STATUS fetch failed");
      }
      // wait release
      while (digitalRead(SW1_PIN) == LOW) yield();
      delay(80);
    }
  }

  // ---- SW2: fetch & apply RGB (red channel), then log to Sheets ----
  if (digitalRead(SW2_PIN) == LOW && millis() - sw2Last > DEBOUNCE_MS) {
    sw2Last = millis();
    delay(30);
    if (digitalRead(SW2_PIN) == LOW) {
      String statusStr; int rgbVal;
      if (fetchStateFromServer(statusStr, rgbVal)) {
        setRed((uint8_t)rgbVal);
        sendToGoogle(String("rgb=") + rgbVal);
        Serial.printf("Applied RGB: %d\n", rgbVal);
      } else {
        Serial.println("RGB fetch failed");
      }
      // wait release
      while (digitalRead(SW2_PIN) == LOW) yield();
      delay(80);
    }
  }
}

/******************* HELPERS *******************/
void applyLed() {
  if (LED_ACTIVE_HIGH) digitalWrite(LED_PIN,  ledState ? HIGH : LOW);
  else                 digitalWrite(LED_PIN,  ledState ? LOW  : HIGH);
}

void setRed(uint8_t val) {
  if (COMMON_ANODE) val = 255 - val;
  analogWrite(R_PIN, val);
}

// Fetch {"status":"ON","rgb":123}  OR  {"led":"ON","rgb":123}
bool fetchStateFromServer(String &ledOut, int &rgbOut) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure(); // OK for coursework; skips CA validation

  HTTPClient http;
  if (!http.begin(client, String(STATUS_URL) + "?_=" + String(millis())))
    return false;

  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String body = http.getString();
  http.end();

  Serial.println("Server JSON: " + body);

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body)) return false;

  // Accept either "status" or "led"
  String s = doc.containsKey("status")
               ? String((const char*)doc["status"])
               : String((const char*)doc["led"]);
  s.toUpperCase();
  if (s != "ON" && s != "OFF") s = "OFF";

  int rgb = doc["rgb"] | 0;
  rgb = constrain(rgb, 0, 255);

  ledOut = s;
  rgbOut = rgb;
  return true;
}

// Send GET to Google Sheets (HTTPS)
bool sendToGoogle(const String& params) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure(); // for class/demo

  HTTPClient http;
  String url = String(GAS_URL) + "?" + params + "&_=" + String(millis());
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  http.end();

  Serial.printf("Sheets GET (%s) -> %d\n", params.c_str(), code);
  return code == 200;
}
