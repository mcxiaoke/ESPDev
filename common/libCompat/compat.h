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
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
// #include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <analogWrite.h>
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

inline size_t fsSize() {
#if defined(ESP8266)
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  return fs_info.totalBytes;
#elif defined(ESP32)
  return SPIFFS.totalBytes();
#endif
}

}  // namespace compat

#endif