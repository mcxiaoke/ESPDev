#ifndef SPIFFS_FILE_READ_SERVER_H
#define SPIFFS_FILE_READ_SERVER_H

#include <Arduino.h>
#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include <FS.h>
#include "utils.h"

struct FileServer {
  static bool handle(AsyncWebServerRequest* request);
};

#endif
