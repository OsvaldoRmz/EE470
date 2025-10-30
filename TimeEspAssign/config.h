#ifndef CONFIG_H
#define CONFIG_H


/* ------------ Pins --------------- */
#define DHTPIN     D6
#define DHTTYPE    DHT11
#define BUTTON_PIN D5
#define LIGHT_PIN  A0
#define TILT_PIN   D7

/* ------------ WiFi / Server ------ */
static const char* WIFI_SSID = "ATT6zmf754";
static const char* WIFI_PASS = "4sgg89n%v7eq";
static const char* SERVER_URL = "https://osvaldoramirezhernandez.com/db2_insert.php";
static const char* TIME_API   = "https://timeapi.io/api/Time/current/zone?timeZone=America/Los_Angeles";

/* ------------ Timing ------------- */
static const int DEBOUNCE_MS = 50;
static const unsigned long DHT_MIN_INTERVAL_MS = 2000; // DHT11 requirement

#endif // CONFIG_H
