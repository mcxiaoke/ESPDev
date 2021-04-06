#ifndef __MCX_LIBMODULES_AWEBSERVER_HEADER__
#define __MCX_LIBMODULES_AWEBSERVER_HEADER__

#include <AFileServer.h>
#include <AModule.h>
#include <AUpdateServer.h>
#include <ESPAsyncWebServer.h>
#include <compat.h>

class AWebServer;

// typedef void (*AWebServerFunc)(AsyncWebServer* server);
typedef std::function<void(AsyncWebServer* server)> AWebServerFunc;

class AWebServer : AModuleInterface {
 private:
  uint16_t port;
  AsyncWebServer server;

 public:
  AWebServer(uint16_t port);
  ~AWebServer();
  AsyncWebServer getServer() { return server; };
  const char* getModuleName() { return "AWebServer"; };
  void setup(AWebServerFunc func);
  bool begin();
  void loop();
};

#endif /* __MCX_LIBMODULES_AWEBSERVER_HEADER__ */
