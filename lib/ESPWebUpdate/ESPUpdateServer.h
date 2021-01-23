#ifndef __ESP_HTTP_UPDATE_SERVER_H__
#define __ESP_HTTP_UPDATE_SERVER_H__

#include <ext/utility.hpp>
#include <utils.h>
#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Update.h>
#endif
#include <ESPAsyncWebServer.h>

class ESPUpdateServer {
 public:
  ESPUpdateServer(bool serial_debug = false,
                  const String& username = emptyString,
                  const String& password = emptyString);

  void setup(AsyncWebServer* server) {
    setup(server, emptyString, emptyString);
  }

  void setup(AsyncWebServer* server, const String& path) {
    setup(server, path, emptyString, emptyString);
  }

  void setup(AsyncWebServer* server,
             const String& username,
             const String& password) {
    setup(server, "/update", username, password);
  }

  void setup(AsyncWebServer* server,
             const String& path,
             const String& username,
             const String& password);

  void updateCredentials(const String& username, const String& password) {
    _username = username;
    _password = password;
  }

 protected:
  void _setUpdaterError();

 private:
  bool _serial_output;
  AsyncWebServer* _server;
  String _username;
  String _password;
  bool _authenticated;
  String _updaterError;
  void handleUpdate(AsyncWebServerRequest* request);
  void handleUploadEnd(AsyncWebServerRequest* request);
  void handleUploadProgress(size_t progress, size_t total);
  void handleUpload(AsyncWebServerRequest* request,
                    const String& filename,
                    size_t index,
                    uint8_t* data,
                    size_t len,
                    bool final);
};

#endif
