#ifndef ESP_DEV_REST_API_H
#define ESP_DEV_REST_API_H

#define ARDUINOJSON_ENABLE_STD_STRING 1

#include <ACommand.h>
#include <ALogger.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <DateTime.h>
#include <ESPAsyncWebServer.h>
#include <RelayUnit.h>
#include <build.h>

#include <algorithm>
#include <tuple>

using RestResponse = std::tuple<int, String>;

class ResetHandler : AsyncWebHandler {
  virtual bool canHandle(AsyncWebServerRequest* request) override final {
    return false;
  }
};

class RestApi {
 public:
  RestApi(const RelayUnit& p);
  void setup(AsyncWebServer* server);
  void handleControl(AsyncWebServerRequest* r);
  void jsonControl(const JsonVariant& json, const String& arguments);
  void jsonHelp(const JsonVariant& json);
  void jsonStatus(const JsonVariant& json);
  void jsonNetwork(const JsonVariant& json);
  void jsonTask(const JsonVariant& json);
  void jsonSimple(const JsonVariant& json);
  void jsonLogs(const JsonVariant& json);
  void jsonFiles(const JsonVariant& json);

 private:
  const RelayUnit& pump;
};

#endif