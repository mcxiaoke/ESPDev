#include <AWebServer.h>

AWebServer::AWebServer(uint16_t _port)
    : port(_port), server(std::make_shared<AsyncWebServer>(_port)) {}

AWebServer::~AWebServer() {}

void AWebServer::setup(AWebServerFunc func) { func(server); }

bool AWebServer::begin() {
  ULOGN(F("[WebServer] Setup Web Sever"));
  ULOGN(F("[WebServer] Add request handler for /"));
  server->on("/", [](AsyncWebServerRequest* request) {
    if (FileFS.exists("/index.html")) {
      request->send(FileFS, "/index.html");
    } else {
      request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
    }
  });
  ULOGN(F("[WebServer] Add request handler for /clear"));
  server->on("/clear", HTTP_POST | HTTP_GET,
             [](AsyncWebServerRequest* request) {
               request->redirect("/");
               LOGN(F("[WebServer] Handing Clear Logs."));
               ALogger.clear();
             });
  ULOGN(F("[WebServer] Add request handler for /reboot"));
  server->on("/reboot", HTTP_POST | HTTP_GET,
             [](AsyncWebServerRequest* request) {
               request->redirect("/");
               LOGN(F("[WebServer] Handling Reboot."));
               compat::restart();
             });
  AUpdateServer.setup(server);
  AUpdateServer.begin();
  AFileServer.setup(server);
  AFileServer.begin();
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), "*");
  DefaultHeaders::Instance().addHeader(F("Cache-Control"), F("no-cache"));
  // DefaultHeaders::Instance().addHeader("Connection", "close");
  server->begin();
  ULOGN(F("[WebServer] Web Sever started"));
  MDNS.addService("http", "tcp", port);
  ULOGN(F("[WebServer] MDNS Service started"));
  return true;
}

void AWebServer::loop() {
  AUpdateServer.loop();
#if defined(ESP8266)
  MDNS.update();
#endif
}