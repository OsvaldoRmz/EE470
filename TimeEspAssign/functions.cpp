#include "functions.h"
#include "config.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DHT.h>

/* ------------ Module state ------- */
static int lastButtonState = HIGH;
static unsigned long lastButtonPress = 0;
static unsigned long lastDhtReadMs   = 0;

/* ------------ DHT ---------------- */
static DHT dht(DHTPIN, DHTTYPE);

/* ------------ Lifecycle ---------- */
void initHardware() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TILT_PIN,   INPUT);
  pinMode(LIGHT_PIN,  INPUT);
  dht.begin();
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
}

/* ------------ Button → DHT -------- */
void check_switch() {
  int currentButton = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentButton == LOW) { // pressed
    if (millis() - lastButtonPress >= (unsigned long)DEBOUNCE_MS) {
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

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // TODO: load proper CA cert in production

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(secureClient, TIME_API)) {
    Serial.println("HTTP begin failed for time API");
    return "";
  }

  String datetime = "";
  int httpCode = http.GET();
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
void tx_lux(String nodeName, String timeStr, int luxRounded) {
  if (timeStr == "" || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  String url = String(SERVER_URL) + "?node_name=" + urlEncode(nodeName) +
               "&lux_value=" + urlEncode(String(luxRounded)) +
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

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  String url = String(SERVER_URL) + "?node_name=" + urlEncode(nodeName) +
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
