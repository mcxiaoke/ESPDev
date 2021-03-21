#ifndef SPIFFS_FILE_READ_SERVER_H
#define SPIFFS_FILE_READ_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif
#include <ALogger.h>

struct FileServer {
  static bool handle(AsyncWebServerRequest* request);
};

#endif
