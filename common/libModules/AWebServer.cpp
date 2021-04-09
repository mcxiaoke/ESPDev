#include <AWebServer.h>

AWebServer::AWebServer(uint16_t _port)
    : port(_port), server(std::make_shared<AsyncWebServer>(_port)) {}

AWebServer::~AWebServer() {}

void AWebServer::setup(AWebServerFunc func) { func(server); }

bool AWebServer::begin() {
  ULOGN("[WebServer] Setup Web Sever");
  ULOGN("[WebServer] Add request handler for /");
  server->on("/", [](AsyncWebServerRequest* request) {
    if (FileFS.exists("/index.html")) {
      request->send(FileFS, "/index.html");
    } else {
      request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
    }
  });
  ULOGN("[WebServer] Add request handler for /clear");
  server->on("/clear", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, "Logs Cleared!");
    LOGN("[WebServer] Handing Clear Logs.");
    ALogger.clear();
  });
  ULOGN("[WebServer] Add request handler for /reboot");
  server->on("/reboot", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, "Reboot Now!");
    LOGN("[WebServer] Handling Reboot.");
    delay(500);
    compat::restart();
  });
  AUpdateServer.setup(server);
  AUpdateServer.begin();
  AFileServer.setup(server);
  AFileServer.begin();
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache");
  server->begin();
  ULOGN("[WebServer] Web Sever started");
  MDNS.addService("http", "tcp", port);
  ULOGN("[WebServer] MDNS Service started");
  return true;
}

void AWebServer::loop() {
  AUpdateServer.loop();
#if defined(ESP8266)
  MDNS.update();
#endif
}