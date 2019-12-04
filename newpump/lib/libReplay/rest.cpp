#include "rest.h"

static JsonDocument filesToJson(
    const std::vector<std::tuple<String, size_t>>& vec) {
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  //   JsonArray data = doc.createNestedArray("data");
  for (auto& v : vec) {
    LOGN("File:", std::get<0>(v), " ", std::get<1>(v));
    JsonObject o = arr.createNestedObject();
    o["name"] = std::get<0>(v);
    o["size"] = std::get<1>(v);
  }
  return doc;
}

static String getCompleteUrl(AsyncWebServerRequest* r) {
  String s = "";
  s += r->url();
  int params = r->params();
  if (params > 0) {
    s += "?";
  }
  for (int i = 0; i < params; i++) {
    auto p = r->getParam(i);
    s += p->name();
    s += "=";
    s += p->value();
    if (i != params) {
      s += "&";
    }
  }
  return s;
}

static void showUrlWithArgs(AsyncWebServerRequest* r) {
  LOGNF("[RestApi] %s: %s", r->methodToString(), getCompleteUrl(r));
}

static void showHeaders(AsyncWebServerRequest* r) {
  //   LOGNF("[RestApi] %s: %s", r->methodToString(), getCompleteUrl(r));
  int headers = r->headers();
  for (int i = 0; i < headers; i++) {
    auto h = r->getHeader(i);
    LOGNF("[HEADER] %s: %s", h->name(), h->value());
  }
}

static String errorResponse(int code, const String& msg, const String& uri) {
  DynamicJsonDocument doc(128);
  doc["code"] = code;
  doc["msg"] = msg;
  doc["uri"] = uri;
  String s;
  serializeJson(doc, s);
  return s;
}

RestApi::RestApi(const RelayUnit& p) : pump(p) {}

void RestApi::setup(AsyncWebServer* server) {
  server->on("/api/status", HTTP_GET, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    auto res = getStatus();
    r->send(200, JSON_MIMETYPE, std::get<2>(res));
  });
  server->on("/api/network", HTTP_GET, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    auto res = getNetwork();
    r->send(200, JSON_MIMETYPE, std::get<2>(res));
  });
  server->on("/api/task", HTTP_GET, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    auto res = getTask();
    r->send(200, JSON_MIMETYPE, std::get<2>(res));
  });
  server->on("/api/logs", HTTP_GET, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    auto res = getLogs();
    r->send(200, JSON_MIMETYPE, std::get<2>(res));
  });
  server->on("/api/files", HTTP_GET, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    auto res = getFiles();
    r->send(200, JSON_MIMETYPE, std::get<2>(res));
  });
  server->on(
      "/api/control", HTTP_POST | HTTP_GET, [&](AsyncWebServerRequest* r) {
        showUrlWithArgs(r);
        AsyncWebParameter* pa;
        (pa = r->getParam("args", true)) || (pa = r->getParam("args"));
        if (pa == nullptr) {
          r->send(400, JSON_MIMETYPE,
                  errorResponse(-1, "Parameter Required: [args]", r->url()));
          return;
        }
        showESP("1111");
        auto res = control(pa->value());
        showESP("2222");
        r->send(200, JSON_MIMETYPE, std::get<2>(res));
        showESP("3333");
      });
}

RestResponse RestApi::getStatus() {
  auto cfg = pump.getConfig();
  auto st = pump.getStatus();
  DynamicJsonDocument doc(1024);
  doc["time"] = DateTime.now();
  doc["ip"] = WiFi.localIP().toString();
  doc["connected"] = compat::isWiFiConnected();
  doc["pin"] = pump.pin();
  doc["on"] = pump.isOn();
  doc["interval"] = cfg->interval / 1000;
  doc["duration"] = cfg->duration / 1000;
  doc["enabled"] = pump.isEnabled();
  doc["last_elapsed"] = st->lastElapsed / 1000;
  doc["total_elapsed"] = st->totalElapsed / 1000;
  doc["boot_time"] = DateTime.getBootTime();
  doc["ms"] = millis() / 1000;
  doc["last_start"] = st->lastStart;
  doc["last_stop"] = st->lastStop;
  doc["last_reset"] = st->timerResetAt;
  doc["heap"] = ESP.getFreeHeap();
  doc["id"] = getUDID();
  String s = "";
  serializeJson(doc, s);
  return std::make_tuple(0, "ok", s);
}
RestResponse RestApi::getNetwork() {
  DynamicJsonDocument doc(128);
  doc["id"] = getUDID();
  doc["mac"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["connected"] = compat::isWiFiConnected();
  doc["ms"] = millis() / 1000;
  doc["time"] = DateTime.now();
  String s = "";
  serializeJson(doc, s);
  return std::make_tuple(0, "ok", s);
}
RestResponse RestApi::getTask() {
  auto t = pump.getRunTask();
  DynamicJsonDocument doc(256);
  doc["id"] = t->id;
  doc["interval"] = t->interval;
  doc["name"] = t->name;
  doc["enabled"] = t->enabled;
  doc["num_runs"] = t->numRuns;
  doc["prev"] = t->prevMillis / 1000;
  doc["ms"] = millis() / 1000;
  doc["time"] = DateTime.now();
  String s = "";
  serializeJson(doc, s);
  return std::make_tuple(0, "ok", s);
}

RestResponse RestApi::getLogs() {
  auto doc = filesToJson(listLogs());
  String s = "";
  serializeJson(doc, s);
  return std::make_tuple(0, "ok", s);
}
RestResponse RestApi::getFiles() {
  auto doc = filesToJson(listFiles());
  String s = "";
  serializeJson(doc, s);
  return std::make_tuple(0, "ok", s);
}
RestResponse RestApi::control(const String& arguments) {
  // format: url?args=cmd,arg1,arg2,arg3
  std::string args(arguments.c_str());
  auto cmd = CommandParam::from(args);
  auto ret = CommandManager.handle(cmd);
  showESP("aaaa");
  DynamicJsonDocument doc(256);
  showESP("bbbb");
  int code = ret ? 0 : -1;
  String msg = ret ? "ok" : "unknown command";
  doc["code"] = code;
  doc["msg"] = msg;
  doc["cmd"] = cmd.name.c_str();
  doc["args"] = ext::string::join(cmd.args).c_str();
  String s = "";
  showESP("cccc");
  serializeJson(doc, s);
  showESP("dddd");
  LOGN("[RestApi] control:", cmd.toString());
  showESP("eee");
  return std::make_tuple(code, msg, s);
}