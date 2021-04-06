#include <AWebServer.h>

AWebServer::AWebServer(uint16_t _port) : port(_port), server(_port) {}

AWebServer::~AWebServer() {}

bool AWebServer::begin() {
  server.on("/", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
  });
  AFileServer.setup(&server);
  AFileServer.begin();
  AUpdateServer.setup(&server);
  AUpdateServer.begin();
  server.begin();
  MDNS.addService("http", "tcp", port);
  return true;
}

void AWebServer::loop() {
  AUpdateServer.loop();
#if defined(ESP8266)
  MDNS.update();
#endif
}