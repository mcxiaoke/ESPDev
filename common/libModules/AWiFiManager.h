/*
 * File: AWiFiManager.hpp
 * Created: 2021-04-05 18:39:03
 * Modified: 2021-04-05 18:45:18
 * Author: mcxiaoke (github@mcxiaoke.com)
 * License: Apache License 2.0
 */

#ifndef __MCX_LIBWIFI_AWIFIMANAGER_HEADER__
#define __MCX_LIBWIFI_AWIFIMANAGER_HEADER__

#define DEFAULT_WIFI_CHECK_INTERVAL 3 * 60 * 1000L

#include <AModule.h>
#include <Arduino.h>
#include <compat.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <ADebug.h>

#define LOG_TAG "[WiFi]"
#define WIFI_CONNECT_TIMEOUT_MS 60 * 1000L

typedef std::function<void(void)> VoidEventFunc;

class AWiFiManagerClass : AModuleInterface {
 private:
  WiFiMode_t mode;
  bool autoConnect;
  bool autoReconnect;
  const char* ssid;
  const char* password;
  unsigned long lastCheckMs;

 protected:
  unsigned long timeoutMs = WIFI_CONNECT_TIMEOUT_MS;
  unsigned long checkIntervalMs = DEFAULT_WIFI_CHECK_INTERVAL;
  VoidEventFunc readyCallback;
  VoidEventFunc lostCallback;
#if defined(ESP8266)
  WiFiEventHandler h0, h1, h2;
#elif defined(ESP32)
  wifi_event_id_t h0, h1, h2;
#endif
  void onConnected();
  void onDisconnected();
  void configEventHandler();
  void checkConnection();

 public:
  AWiFiManagerClass(WiFiMode_t _mode = WIFI_STA);
  ~AWiFiManagerClass();
  const char* getModuleName() { return "AWiFiManager"; }
  void setCredentials(const char* _ssid, const char* _password);
  bool begin();
  void loop();
  void setTimeout(unsigned long _timeoutMs);
  void onWiFiReady(VoidEventFunc func) { readyCallback = func; }
  void onWiFiLost(VoidEventFunc func) { lostCallback = func; }
};

extern AWiFiManagerClass AWiFi;  // define in cpp

#endif /* __MCX_LIBWIFI_AWIFIMANAGER_HEADER__ */
