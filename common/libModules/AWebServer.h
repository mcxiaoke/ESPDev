#ifndef __MCX_LIBMODULES_AWEBSERVER_HEADER__
#define __MCX_LIBMODULES_AWEBSERVER_HEADER__

#include <AFileServer.h>
#include <AModule.h>
#include <AUpdateServer.h>
#include <ESPAsyncWebServer.h>
#include <SafeMode.h>
#include <compat.h>

#include <memory>

class AWebServer;

// typedef void (*AWebServerFunc)(AsyncWebServer* server);
typedef std::function<void(std::shared_ptr<AsyncWebServer> server)>
    AWebServerFunc;

class AWebServer : public AModuleInterface {
 private:
  uint16_t port;
  std::shared_ptr<AsyncWebServer> server;

 public:
  AWebServer(uint16_t port);
  ~AWebServer();
  std::shared_ptr<AsyncWebServer> getServer() { return server; };
  const char* getModuleName() { return "AWebServer"; };
  void setup(AWebServerFunc func);
  bool begin();
  void loop();
};

#endif /* __MCX_LIBMODULES_AWEBSERVER_HEADER__ */
