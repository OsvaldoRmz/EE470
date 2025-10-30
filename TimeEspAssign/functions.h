#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>

// lifecycle
void initHardware();
void connectWiFi();

// behavior (called by loop)
void check_switch();   // Button -> DHT read & TX (node_2)
void read_sensor_2();  // Tilt   -> Lux read & TX (node_1)

// helpers (exposed in case you want to reuse)
String read_time();
String normalizeTimeForMySQL(const String &isoLocal);
String urlEncode(const String &value);
void   tx_lux(String nodeName, String timeStr, int luxRounded);
void   tx_dht(String nodeName, String timeStr, float tempC, float humidity);

#endif // FUNCTIONS_H
