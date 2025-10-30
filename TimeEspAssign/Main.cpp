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
 */






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
