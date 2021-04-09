#ifndef ESP_DEV_FILE_SERVER_H
#define ESP_DEV_FILE_SERVER_H

#include <ADebug.h>
#include <AFileUtils.h>
#include <AModule.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <compat.h>

constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";
// {"code":404,"msg":"resource not found","uri":"/api/notfound"}

class AFileServerClass : public AModuleInterface {
 public:
  void setup(std::shared_ptr<AsyncWebServer> server);
  bool handle(AsyncWebServerRequest* request);
  bool begin();
  void loop(){};
  const char* getModuleName() { return "FileServer"; }
};

extern AFileServerClass AFileServer;

#endif
