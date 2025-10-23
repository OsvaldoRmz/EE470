/*
 * ----------------------------------------------
 * Project/Program Name : IoT Sensor Database Logger
 * File Name            : db2_insert.php
 * Author               : Osvaldo Ramirez
 * Date                 : 23/10/2025
 * Version              : 1.0
 * 
 * Purpose:
 *    This program receives sensor data sent from an ESP8266
 *    over HTTP (via GET or POST) and inserts it into a MySQL database.
 *    It supports multiple sensor nodes sending temperature, humidity,
 *    and light (Lux) values with timestamps for real-time logging.
 * 
 * Inputs:
 *    - node_name:  identifies which ESP8266 node sent the data.
 *    - lux_value:  analog light sensor reading (Lux).
 *    - temp_c:     temperature in Celsius from a DHT sensor.
 *    - humidity:   relative humidity (%) from a DHT sensor.
 *    - time_received: timestamp string from the ESP 
 * 
 * Outputs:
 *    - Inserts a new record into the MySQL table 'sensor_data_new'.
 *    - Returns a JSON confirmation message with insertion details.
 * 
 * Dependencies:
 *    - PHP PDO extension for MySQL database access.
 *    - Active MySQL database with table 'sensor_data_new'.
 *    - WiFi-connected ESP8266/NodeMCU microcontroller.
 * 
 * Usage Notes:
 *    - Modify DB credentials (DB_HOST, DB_USER, DB_PASS, DB_NAME)
 *      to match your hosting setup.
 *    - Ensure CORS and permissions are configured for remote inserts.
 *    - This file must be uploaded to a server accessible to the ESP8266.
 * 
 * ---------------------------------------------------------------------------
 */








#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DHT.h>

/* ------------ Pins --------------- */
#define DHTPIN     D6       // KY-015 signal pin
#define DHTTYPE    DHT11    // <-- KY-015 uses DHT11
#define BUTTON_PIN D5       // same as your original
#define LIGHT_PIN  A0
#define TILT_PIN   D7

/* ------------ WiFi / Server ------ */
const char* ssid     = "ATT6zmf754";
const char* password = "4sgg89n%v7eq";
String serverURL     = "https://osvaldoramirezhernandez.com/db2_insert.php"; // use HTTPS

/* ------------ Vars ---------------- */
int lastButtonState = HIGH;
unsigned long lastButtonPress = 0;
const int DEBOUNCE_MS = 50;
unsigned long lastDhtReadMs = 0;  // rate limit for DHT
const unsigned long DHT_MIN_INTERVAL_MS = 2000; // DHT11 requirement

/* ------------ DHT ----------------- */
DHT dht(DHTPIN, DHTTYPE);

/* ------------ Prototypes ---------- */
void check_switch();
void read_sensor_2();
String read_time();
String normalizeTimeForMySQL(const String &isoLocal);
String urlEncode(const String &value);
void tx_lux(String nodeName, String timeStr, int lightState);
void tx_dht(String nodeName, String timeStr, float tempC, float humidity);

/* ------------ Setup --------------- */
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TILT_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
}

/* ------------ Loop ---------------- */
void loop() {
  check_switch();   // Button: read DHT + send temp/humidity (node_2)
  read_sensor_2();  // Tilt: read lux + send lux (node_1)
  delay(50);
}

/* ------------ Button → DHT -------- */
void check_switch() {
  int currentButton = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButton == LOW) { // pressed
    if (millis() - lastButtonPress >= DEBOUNCE_MS) {
      lastButtonPress = millis();

      // obey DHT11 minimum interval
      if (millis() - lastDhtReadMs < DHT_MIN_INTERVAL_MS) {
        delay(DHT_MIN_INTERVAL_MS - (millis() - lastDhtReadMs));
      }
      float h = dht.readHumidity();
      float t = dht.readTemperature(); // Celsius
      if (isnan(h) || isnan(t)) {      // retry once
        delay(150);
        h = dht.readHumidity();
        t = dht.readTemperature();
      }
      lastDhtReadMs = millis();

      if (isnan(h) || isnan(t)) {
        Serial.println("DHT11 read failed");
      } else {
        String timeStr = "";
        while (timeStr == "") { timeStr = read_time(); if (timeStr == "") delay(100); }
        String mysqlTime = normalizeTimeForMySQL(timeStr);
        tx_dht("node_2", mysqlTime, t, h);
      }
    }
  }
  lastButtonState = currentButton;
}

/* ------------ Tilt → Lux ---------- */
void read_sensor_2() {
  int tilt = digitalRead(TILT_PIN);
  if (tilt == HIGH) { // tilt detected
    int adc = analogRead(LIGHT_PIN);
    // Map ADC (0–1023) → Lux (0–1000), inverse relation
    float lux = (1023.0 - adc) * (1000.0 / 1023.0);
    if (lux < 0) lux = 0;

    // --- get time + transmit ---
    String timeStr = "";
    while (timeStr == "") { timeStr = read_time(); if (timeStr == "") delay(100); }
    String mysqlTime = normalizeTimeForMySQL(timeStr);

    tx_lux("node_1", mysqlTime, (int)round(lux));

    Serial.printf("ADC=%d  Lux≈%.1f\n", adc, lux);
  }
}
/* ------------ Time (HTTPS) -------- */
String read_time() {
  if (WiFi.status() != WL_CONNECTED) return "";
  WiFiClientSecure secureClient; secureClient.setInsecure(); // load CA in production
  HTTPClient http; http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  const char* url = "https://timeapi.io/api/Time/current/zone?timeZone=America/Los_Angeles";

  if (!http.begin(secureClient, url)) { Serial.println("HTTP begin failed for time API"); return ""; }
  int httpCode = http.GET();
  String datetime = "";
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      datetime = doc["dateTime"].as<String>(); // e.g. 2025-10-23T00:14:59.6799641
    } else {
      Serial.println("JSON parse error (time API)");
    }
  } else {
    Serial.printf("HTTP error getting time: %d\n", httpCode);
  }
  http.end();
  return datetime;
}

/* ------------ Normalize time ------ */
String normalizeTimeForMySQL(const String &isoLocal) {
  String s = isoLocal;
  s.replace("T", " ");
  int dot = s.indexOf('.');
  if (dot >= 0) s = s.substring(0, dot);
  if (s.endsWith("Z")) s = s.substring(0, s.length() - 1);
  return s; // "YYYY-MM-DD HH:MM:SS"
}

/* ------------ URL encode ---------- */
String urlEncode(const String &value) {
  String out = ""; const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') out += c;
    else { out += '%'; out += hex[(c>>4)&0xF]; out += hex[c&0xF]; }
  }
  return out;
}

/* ------------ Transmit (GET) ------ */
void tx_lux(String nodeName, String timeStr, int lightState) {
  if (timeStr == "" || WiFi.status() != WL_CONNECTED) return;
  HTTPClient http; http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String url = serverURL + "?node_name=" + urlEncode(nodeName) +
               "&lux_value=" + urlEncode(String(lightState)) +
               "&time_received=" + urlEncode(timeStr);
  Serial.println("GET URL: " + url);
  WiFiClientSecure s; s.setInsecure();
  if (http.begin(s, url)) {
    int code = http.GET();
    Serial.printf("Server response (%d): %s\n", code, http.getString().c_str());
    http.end();
  } else {
    Serial.println("HTTP begin failed (HTTPS) for lux");
  }
}

void tx_dht(String nodeName, String timeStr, float tempC, float humidity) {
  if (timeStr == "" || WiFi.status() != WL_CONNECTED) return;
  HTTPClient http; http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String url = serverURL + "?node_name=" + urlEncode(nodeName) +
               "&temp_c=" + urlEncode(String(tempC, 2)) +
               "&humidity=" + urlEncode(String(humidity, 2)) +
               "&time_received=" + urlEncode(timeStr);
  Serial.println("GET URL: " + url);
  WiFiClientSecure s; s.setInsecure();
  if (http.begin(s, url)) {
    int code = http.GET();
    Serial.printf("Server response (%d): %s\n", code, http.getString().c_str());
    http.end();
  } else {
    Serial.println("HTTP begin failed (HTTPS) for dht");
  }
}

