#include <AWebServer.h>

AWebServer::AWebServer(uint16_t _port)
    : port(_port), server(std::make_shared<AsyncWebServer>(_port)) {}

AWebServer::~AWebServer() {}

void AWebServer::setup(AWebServerFunc func) { func(server); }

bool AWebServer::begin() {
  server->on("/", [](AsyncWebServerRequest* request) {
    if (FileFS.exists("/index.html")) {
      request->send(FileFS, "/index.html");
    } else {
      request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
    }
  });
  server->on("/reboot", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, "OK");
    compat::restart();
  });
  AUpdateServer.setup(server);
  AUpdateServer.begin();
  AFileServer.setup(server);
  AFileServer.begin();
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  server->begin();
  MDNS.addService("http", "tcp", port);
  return true;
}

void AWebServer::loop() {
  AUpdateServer.loop();
#if defined(ESP8266)
  MDNS.update();
#endif
}