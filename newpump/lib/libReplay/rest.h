#ifndef ESP_DEV_REST_API_H
#define ESP_DEV_REST_API_H

#include <string>
#include <ESPAsyncWebServer.h>

struct Response {
  const int statusCode;
  const std::string body;
};

class RestApi {
 public:
  String getStatus();
  String getNetwork();
  String getDevice();
  String getTask();
  String getTimer();
  String getLogs();
  String getFiles();
  String control();
};

#endif