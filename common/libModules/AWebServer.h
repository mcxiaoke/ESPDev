#ifndef __MCX_LIBMODULES_AWEBSERVER_HEADER__
#define __MCX_LIBMODULES_AWEBSERVER_HEADER__

#include <AFileServer.h>
#include <AModule.h>
#include <AUpdateServer.h>
#include <ESPAsyncWebServer.h>
#include <compat.h>

class AWebServer : AModuleInterface {
 private:
  uint16_t port;
  AsyncWebServer server;

 public:
  AWebServer(uint16_t port);
  ~AWebServer();
  const char* getModuleName() { return "AWebServer"; };
  bool begin();
  void loop();
};

#endif /* __MCX_LIBMODULES_AWEBSERVER_HEADER__ */
