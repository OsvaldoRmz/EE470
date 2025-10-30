#include "config.h"
#include "functions.h"
#include "Arduino.h"
void setup() {
  initHardware();
  connectWiFi();
}

void loop() {
  check_switch();   // Button → DHT
  read_sensor_2();  // Tilt   → Lux
  delay(50);
}
