#include <AWebServer.h>

AWebServer::AWebServer(uint16_t _port)
    : port(_port), server(std::make_shared<AsyncWebServer>(_port)) {}

AWebServer::~AWebServer() {}

void AWebServer::setup(AWebServerFunc func) {
  func(server);
}

bool AWebServer::begin() {
  ULOGN(F("[WebServer] Setup Web Sever"));
  AUpdateServer.setup(server);
  AUpdateServer.begin();
  AFileServer.setup(server);
  AFileServer.begin();
  ULOGN(F("[WebServer] Add handler for /safe_mode/1"));
  server->on("/safe_mode/1", HTTP_POST | HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               request->redirect("/");
               if (SafeMode.setEnable(true)) {
                 LOGN(F("[WebServer] Enable Safe Mode."));
                 setShouldRestart(true);
               }
             });
  delay(5);
  ULOGN(F("[WebServer] Add handler for /safe_mode/0"));
  server->on("/safe_mode/0", HTTP_POST | HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               request->redirect("/");
               if (SafeMode.setEnable(false)) {
                 LOGN(F("[WebServer] Disable Safe Mode."));
                 setShouldRestart(true);
               }
             });
  ULOGN(F("[WebServer] Add handler for /logs"));
  server->on("/logs", [](AsyncWebServerRequest* request) {
    request->send(FileFS, "/serial.log", "text/plain");
  });
  delay(5);
  ULOGN(F("[WebServer] Add handler for /clear"));
  server->on("/clear", HTTP_POST | HTTP_GET,
             [](AsyncWebServerRequest* request) {
               request->redirect("/");
               LOGN(F("[WebServer] Handing Clear Logs."));
               ALogger.clear();
             });
  delay(5);
  ULOGN(F("[WebServer] Add handler for /reboot"));
  server->on("/reboot", HTTP_POST | HTTP_GET,
             [this](AsyncWebServerRequest* request) {
               request->redirect("/");
               ULOGN(F("[WebServer] Handling Reboot."));
               setShouldRestart(true);
             });
  delay(5);
  ULOGN(F("[WebServer] Add handler for /"));
  server->on("/", [](AsyncWebServerRequest* request) {
    if (FileFS.exists("/index.html")) {
      request->send(FileFS, "/index.html");
    } else {
      request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
    }
  });
  delay(5);
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), "*");
  DefaultHeaders::Instance().addHeader(F("Cache-Control"), F("no-cache"));
  // DefaultHeaders::Instance().addHeader("Connection", "close");
  server->begin();
  LOGN(F("[WebServer] Web Sever started"));
  if (!MDNS.begin(compat::getHostName())) {
    LOGN("[WebServer] Cannot start mDNS responder");
  } else {
    MDNS.addService("http", "tcp", port);
    LOGN("[WebServer] mDNS responder started");
  }

  return true;
}

void AWebServer::loop() {
#if defined(ESP8266)
  MDNS.update();
#endif
  if (shouldRestart()) {
    LOGN("[WebServer] Safe Mode changed, Reboot.");
    setShouldRestart(false);
    delay(1000);
    compat::restart();
  }
  AUpdateServer.loop();
}