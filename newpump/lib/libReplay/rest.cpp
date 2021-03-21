#include "rest.h"

static constexpr const char* REST_TOKEN = "pump";
static constexpr const char* SKIP_CMD[] = {"help",   "files", "logs",
                                           "status", "wifi",  "list"};
static void filesToJson(const std::vector<std::tuple<String, size_t>>& vec,
                        const JsonVariant& doc) {
  JsonArray arr = doc.to<JsonArray>();
  for (auto& v : vec) {
    LOGN("File:", std::get<0>(v), std::get<1>(v));
    JsonObject o = arr.createNestedObject();
    if (std::get<0>(v).length() > 0 && std::get<1>(v) > 0) {
      o["n"] = std::get<0>(v);
      o["z"] = std::get<1>(v);
    }
  }
}

static String getCompleteUrl(AsyncWebServerRequest* r) {
  String s = "";
  s += r->url();
  int pc = r->params();
  if (pc > 0) {
    s += "?";
  }
  for (int i = 0; i < pc; i++) {
    auto p = r->getParam(i);
    s += p->name();
    s += "=";
    s += p->value();
    if (i != pc - 1) {
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
    LOGNF("[RestApi][HEADER] %s: %s", h->name(), h->value());
  }
}

static AsyncJsonResponse* buildResponse(
    std::function<void(const JsonVariant&)> prepareFunc, bool isArray = false) {
  auto res = new AsyncJsonResponse(isArray);
  auto root = res->getRoot();
  prepareFunc(root);
  res->setLength();
  return res;
}

static AsyncJsonResponse* errorResponse(int code, const String& msg,
                                        const String& uri) {
  auto func = [&](const JsonVariant& doc) {
    doc["code"] = code;
    doc["msg"] = msg;
    doc["uri"] = uri;
  };
  return buildResponse(func);
}

static bool shouldSkipCmd(const char* cmd) {
  for (auto i = 0; i < std::end(SKIP_CMD) - std::begin(SKIP_CMD); i++) {
    if (strcmp(SKIP_CMD[i], cmd) == 0) {
      return true;
    }
  }
  return false;
}

RestApi::RestApi(const RelayUnit& p) : pump(p) {}

void RestApi::setup(AsyncWebServer* server) {
  LOGN("RestApi::setup()");
  auto names = CommandManager.getCommandNames();
  for (auto name : names) {
    if (!shouldSkipCmd(name.c_str())) {
      auto f = "/api/" + name;
      auto t = "/api/control?cmd=" + name;
      //   LOGN("RestApi::addRewrite", f, t);
      server->addRewrite(new AsyncWebRewrite(f.c_str(), t.c_str()));
    }
  }
  server->on("/api/help", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    r->send(buildResponse(
        std::bind(&RestApi::jsonHelp, this, std::placeholders::_1), true));
  });
  server->on(
      "/api/status", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
        // showUrlWithArgs(r);
        r->send(buildResponse(
            std::bind(&RestApi::jsonStatus, this, std::placeholders::_1)));
      });
  server->on(
      "/api/network", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
        showUrlWithArgs(r);
        r->send(buildResponse(
            std::bind(&RestApi::jsonNetwork, this, std::placeholders::_1)));
      });
  server->on("/api/task", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    r->send(buildResponse(
        std::bind(&RestApi::jsonTask, this, std::placeholders::_1)));
  });
  server->on("/api/logs", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    r->send(buildResponse(
        std::bind(&RestApi::jsonLogs, this, std::placeholders::_1)));
  });
  server->on("/api/files", HTTP_GET | HTTP_POST, [&](AsyncWebServerRequest* r) {
    showUrlWithArgs(r);
    // showHeaders(r);
    r->send(buildResponse(
        std::bind(&RestApi::jsonFiles, this, std::placeholders::_1)));
  });
  server->on("/api/control", HTTP_POST | HTTP_GET,
             [&](AsyncWebServerRequest* r) {
               showUrlWithArgs(r);
               //  showHeaders(r);
               handleControl(r);
             });
}

void RestApi::handleControl(AsyncWebServerRequest* r) {
  AsyncWebParameter* pa = nullptr;
  (pa = r->getParam("args", true)) || (pa = r->getParam("args")) ||
      (pa = r->getParam("cmd", true)) || (pa = r->getParam("cmd"));
  String cmd = emptyString;
  if (pa == nullptr || pa->value() == emptyString) {
    cmd = r->header("Command");
  } else {
    cmd = pa->value();
  }
  LOGNF("RestApi::handleControl: cmd=[%s]", cmd);
  if (cmd == emptyString) {
    auto res = errorResponse(
        -4, "Missing Parameter: [cmd] or Header: [Command]", getCompleteUrl(r));
    res->setCode(400);
    r->send(res);
    return;
  }
  AsyncWebParameter* pt = nullptr;
  (pt = r->getParam("token", true)) || (pt = r->getParam("token"));
  if (pt == nullptr || pt->value() == emptyString) {
    auto res = errorResponse(-9, "Missing Token: [token]", getCompleteUrl(r));
    res->setCode(401);
    r->send(res);
    return;
  }
  //   LOGNF("RestApi::handleControl: token=[%s]", pt->value());
  //   if (pt->value() != REST_TOKEN) {
  //     auto res = errorResponse(-8, "Invalid Token: [token]",
  //     getCompleteUrl(r)); res->setCode(403); r->send(res); return;
  //   }
  //   buildResponse([&](const JsonVariant& json) {
  //     json["uri"] = getCompleteUrl(r);
  //     jsonControl(json, cmd);
  //   });
  //   r->redirect(r->header("Referer"));
  r->send(buildResponse([&](const JsonVariant& json) {
    json["uri"] = getCompleteUrl(r);
    jsonControl(json, cmd);
  }));
}

void RestApi::jsonControl(const JsonVariant& doc, const String& arguments) {
  LOGN("RestApi::jsonControl", arguments);
  // format: url?args=cmd,arg1,arg2,arg3
  std::string args(arguments.c_str());
  if (ext::string::trim(args).empty()) {
    doc["code"] = -2;
    doc["msg"] = "invalid command";
  } else {
    auto cmd = CommandParam::from(args);
    // cmd.callback = [](const CommandResult& ret) {};
    auto ret = CommandManager.handle(cmd);
    int code = ret ? 0 : -1;  // means not found
    String msg = ret ? "ok" : "unknown command";
    doc["code"] = code;
    doc["msg"] = msg;
    // String.c_str() may not valid const char* str
    // and ArduinoJson not accept std::string
    // so use String(stdstr::c_str())
    // or #define ARDUINOJSON_ENABLE_STD_STRING 1
    doc["cmd"] = cmd.name;
    doc["args"] = ext::string::join(cmd.args);
  }
}

void RestApi::jsonHelp(const JsonVariant& json) {
  auto cmds = CommandManager.getCommands();
  for (auto cp : cmds) {
    json.add(cp->name + " - " + cp->desc);
  }
}

void RestApi::jsonStatus(const JsonVariant& doc) {
  auto cfg = pump.getConfig();
  auto st = pump.getStatus();
  doc["time"] = DateTime.now();
  doc["ip"] = WiFi.localIP().toString();
  doc["connected"] = compat::isWiFiConnected();
  doc["pin"] = pump.pin();
  doc["on"] = pump.isOn();
  doc["interval"] = cfg->interval / 1000;
  doc["duration"] = cfg->duration / 1000;
  doc["enabled"] = pump.isTimerEnabled();
  doc["last_elapsed"] = st->lastElapsed / 1000;
  doc["total_elapsed"] = st->totalElapsed / 1000;
  doc["boot_time"] = DateTime.getBootTime();
  doc["up_time"] = millis() / 1000;
  doc["last_start"] = st->lastStart / 1000;
  doc["last_stop"] = st->lastStop / 1000;
  doc["last_reset"] = st->timerResetAt / 1000;
  doc["heap"] = ESP.getFreeHeap();
  doc["device"] = getUDID();
  doc["chip_id"] = ESP.getChipId();
  doc["sketch"] = ESP.getSketchMD5().substring(0, 8);
#ifdef APP_BUILD
  doc["version"] = APP_BUILD;
#endif
#ifdef APP_REVISION
  doc["revision"] = APP_REVISION;
#endif
#ifdef DEBUG
  doc["debug"] = 1;
#else
  doc["debug"] = 0;
#endif
  auto task = doc.createNestedObject("task");
  jsonTask(task);
}
void RestApi::jsonNetwork(const JsonVariant& doc) {
  doc["device"] = getUDID();
  doc["mac"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["connected"] = compat::isWiFiConnected();
  doc["up_time"] = millis() / 1000;
  doc["time"] = DateTime.now();
}
void RestApi::jsonTask(const JsonVariant& doc) {
  auto t = pump.getRunTask();
  doc["id"] = t->id;
  doc["interval"] = t->interval / 1000;
  doc["name"] = t->name;
  doc["enabled"] = t->enabled;
  doc["num_runs"] = t->numRuns;
  doc["start"] = t->startMillis / 1000;
  doc["prev"] = t->prevMillis / 1000;
  doc["up_time"] = millis() / 1000;
  doc["time"] = DateTime.now();
}

void RestApi::jsonLogs(const JsonVariant& json) {
  filesToJson(listLogs(), json);
}
void RestApi::jsonFiles(const JsonVariant& json) {
  filesToJson(listFiles(), json);
}
