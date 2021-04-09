#ifndef __ESP_HTTP_UPDATE_SERVER_H__
#define __ESP_HTTP_UPDATE_SERVER_H__

#define FIRMWARE_UPDATE_FILE "/firmware_update.txt"

#include <ADebug.h>
#include <AModule.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SafeMode.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <compat.h>

#include <memory>

class AUpdateServerClass : public AModuleInterface {
 public:
  AUpdateServerClass(bool serial_debug = false,
                     const String& username = emptyString,
                     const String& password = emptyString);

  void setup(std::shared_ptr<AsyncWebServer> server) {
    setup(server, emptyString, emptyString);
  }

  void setup(std::shared_ptr<AsyncWebServer> server, const String& path) {
    setup(server, path, emptyString, emptyString);
  }

  void setup(std::shared_ptr<AsyncWebServer> server, const String& username,
             const String& password) {
    setup(server, "/update", username, password);
  }

  void setup(std::shared_ptr<AsyncWebServer> server, const String& path,
             const String& username, const String& password);

  void updateCredentials(const String& username, const String& password) {
    _username = username;
    _password = password;
  }

  bool begin();
  void loop();
  const char* getModuleName() { return "ESPUpdateServer"; }

 protected:
  void _setUpdaterError();

 private:
  std::shared_ptr<AsyncWebServer> _server;
  String _username;
  String _password;
  String _path;
  String _updaterError;
  unsigned long progressMs;
  void handleUpdatePage(AsyncWebServerRequest* request);
  void handleUploadEnd(AsyncWebServerRequest* request);
  void handleUploadProgress(size_t progress, size_t total);
  void handleUpload(AsyncWebServerRequest* request, const String& filename,
                    size_t index, uint8_t* data, size_t len, bool final);
};

extern AUpdateServerClass AUpdateServer;

#endif
