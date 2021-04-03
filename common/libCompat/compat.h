#ifndef ESP_DEV_COMPAT_H
#define ESP_DEV_COMPAT_H

#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
// #include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include <flash_hal.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <Update.h>
// #include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
// #include <analogWrite.h>
#endif

#if USE_LITTLEFS
#include <LittleFS.h>
#define FileFS LittleFS
#define FS_Name "LittleFS"
#else
#define FileFS SPIFFS
#define FS_Name "SPIFFS"
#endif

namespace compat {

inline bool isWiFiConnected() {
#ifdef ESP8266
  return WiFi.isConnected();
#elif ESP32
  return WiFi.isConnected();
#endif
}

inline void setHostname(const char* hostname) {
#if defined(ESP8266)
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.hostname(hostname);
#elif defined(ESP32)
  WiFi.setHostname(hostname);
#endif
}

inline size_t flashSize() {
#if defined(ESP8266)
  return ((size_t)&_FS_end - (size_t)&_FS_start);
#elif defined(ESP32)
  return FileFS.totalBytes();
#endif
}

inline void restart() {
#if defined(ESP8266)
  ESP.restart();
#elif defined(ESP32)
  ESP.restart();
#endif
}

}  // namespace compat

#endif