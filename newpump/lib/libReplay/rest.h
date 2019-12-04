#ifndef ESP_DEV_REST_API_H
#define ESP_DEV_REST_API_H

#include <tuple>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ALogger.h>
#include <RelayUnit.h>
#include <ACommand.h>
#include <DateTime.h>
#include <AsyncJson.h>

using RestResponse = std::tuple<int, String, String>;

class ResetHandler : AsyncWebHandler {
  virtual bool canHandle(AsyncWebServerRequest* request) override final {
    return false;
  }
};

class RestApi {
 public:
  RestApi(const RelayUnit& p);
  void setup(AsyncWebServer* server);
  RestResponse getStatus();
  RestResponse getNetwork();
  RestResponse getTask();
  RestResponse getLogs();
  RestResponse getFiles();
  RestResponse control(const String& arguments);

 private:
  const RelayUnit& pump;
};

#endif