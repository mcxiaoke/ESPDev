#ifndef SPIFFS_FILE_READ_SERVER_H
#define SPIFFS_FILE_READ_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

struct FileServer {
  static bool handle(AsyncWebServerRequest* request);
};

#endif
