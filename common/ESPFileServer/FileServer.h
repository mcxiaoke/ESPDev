#ifndef ESP_DEV_FILE_SERVER_H
#define ESP_DEV_FILE_SERVER_H

#include <ALogger.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <compat.h>

struct FileServer {
  static bool handle(AsyncWebServerRequest* request);
};

#endif
