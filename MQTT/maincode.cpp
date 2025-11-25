#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ========= USER SETTINGS =========
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_SERVER   = "broker.hivemq.com";
const uint16_t MQTT_PORT  = 1883;

// Topics (change xxx to your own unique code if you want)
const char* TOPIC_SUB_LED = "testtopic/temp/inTopic";
const char* TOPIC_PUB_OUT = "testtopic/temp/outTopic/ramirezheo";

// Pins
const int POT_PIN    = A0;   // potentiometer
const int LED_PIN    = D5;   // LED
const int SWITCH_PIN = D6;   // momentary button to GND (INPUT_PULLUP)

// Timing
const unsigned long POT_INTERVAL_MS   = 15000; // 15 s
const unsigned long SWITCH_HIGH_MS    = 5000;  // 5 s after sending "1"

// Globals
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPotSend = 0;

bool lastSwitchState   = HIGH;  // INPUT_PULLUP: HIGH = not pressed
bool sentSwitchOne     = false;
unsigned long switchOneTime = 0;

// ====== WiFi ======
void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ====== MQTT callback (LED control) ======
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  if (String(topic) == TOPIC_SUB_LED) {
    if (msg == "1") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ON (via MQTT)");
    } else if (msg == "0") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED OFF (via MQTT)");
    }
  }
}

// ====== MQTT reconnect ======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    // no username/password with public broker.hivemq.com
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(TOPIC_SUB_LED);
      Serial.print("Subscribed to: ");
      Serial.println(TOPIC_SUB_LED);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ====== Setup / Loop ======
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);

  setupWifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);

  randomSeed(analogRead(A0));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // ---- 1) Periodic potentiometer publish ----
  if (now - lastPotSend >= POT_INTERVAL_MS) {
    lastPotSend = now;
    int raw = analogRead(POT_PIN); // 0..1023
    char payload[16];
    snprintf(payload, sizeof(payload), "%d", raw);

    Serial.print("Publishing pot value: ");
    Serial.println(payload);

    client.publish(TOPIC_PUB_OUT, payload);
  }

  // ---- 2) Switch press & release handling ----
  bool currentSwitchState = digitalRead(SWITCH_PIN); // HIGH = not pressed, LOW = pressed

  // detect release: was LOW (pressed), now HIGH (released)
  if (lastSwitchState == LOW && currentSwitchState == HIGH) {
    Serial.println("Switch pressed & released -> sending 1");
    client.publish(TOPIC_PUB_OUT, "1");
    sentSwitchOne = true;
    switchOneTime = now;
  }

  lastSwitchState = currentSwitchState;

  // after 5 seconds, send "0" once
  if (sentSwitchOne && (now - switchOneTime >= SWITCH_HIGH_MS)) {
    Serial.println("5 seconds elapsed -> sending 0");
    client.publish(TOPIC_PUB_OUT, "0");
    sentSwitchOne = false;
  }
}
