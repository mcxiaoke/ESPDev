#ifndef _REST_API_HANDLE__
#define _REST_API_HANDLE__

#include <string>
#include "libs/compat.h"
#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

using namespace std;

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