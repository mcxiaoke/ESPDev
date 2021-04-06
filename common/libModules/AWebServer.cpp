#include <AWebServer.h>

static String getFilesHtml() {
  auto items = listFiles();
  String html = "<ul>";
  for (auto const& i : items) {
    html += "<li><a href='";
    html += std::get<0>(i);
    html += "' target='_blank' >";
    html += std::get<0>(i);
    html += " (";
    html += std::get<1>(i);
    html += " bytes)</a></li>\n";
  }
  html += "</ul>";
  return html;
}

AWebServer::AWebServer(uint16_t _port)
    : port(_port), server(std::make_shared<AsyncWebServer>(_port)) {}

AWebServer::~AWebServer() {}

void AWebServer::setup(AWebServerFunc func) { func(server); }

bool AWebServer::begin() {
  server->on("/", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, compat::getHostName());
  });
  server->on("/reboot", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_PLAIN, "OK");
    compat::restart();
  });
  server->on("/logs", [](AsyncWebServerRequest* request) {
    // request->send(FileFS, "/serial.log", "text/plain");
    AsyncWebServerResponse* response =
        request->beginResponse(FileFS, "/serial.log", "text/plain");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });
  server->on("/files", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_HTML, getFilesHtml());
  });
  // AUpdateServer.setup(server);
  // AUpdateServer.begin();
  server->onNotFound([this](AsyncWebServerRequest* request) {
    if (!AFileServer.handle(request)) {
      String data = F("ERROR: NOT FOUND\nURI: ");
      data += request->url();
      data += "\n";
      request->send(404, MIME_TEXT_PLAIN, data);
    }
  });
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