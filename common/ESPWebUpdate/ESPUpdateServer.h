#ifndef __ESP_HTTP_UPDATE_SERVER_H__
#define __ESP_HTTP_UPDATE_SERVER_H__

#define FIRMWARE_UPDATE_FILE "/firmware_update.txt"

#include <Arduino.h>
#include <ArduinoTimer.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <compat.h>
#include <utils.h>
#include <ALogger.h>

#include <ext/utility.hpp>

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

  void setup(AsyncWebServer* server, const String& username,
             const String& password) {
    setup(server, "/update", username, password);
  }

  void setup(AsyncWebServer* server, const String& path, const String& username,
             const String& password);

  void updateCredentials(const String& username, const String& password) {
    _username = username;
    _password = password;
  }

  void loop();

 protected:
  void _setUpdaterError();

 private:
  bool _serial_output;
  bool _shouldRestart;
  AsyncWebServer* _server;
  String _username;
  String _password;
  bool _authenticated;
  String _updaterError;
  unsigned long progressMs;
  void handleUpdatePage(AsyncWebServerRequest* request);
  void handleUploadEnd(AsyncWebServerRequest* request);
  void handleUploadProgress(size_t progress, size_t total);
  void handleUpload(AsyncWebServerRequest* request, const String& filename,
                    size_t index, uint8_t* data, size_t len, bool final);
};

#endif
