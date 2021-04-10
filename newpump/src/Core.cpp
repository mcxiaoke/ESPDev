#include "Core.h"

RelayUnit pump;
RestApi api(pump);

void statusReport();
void handleCommand(const CommandParam& param);

String getFilesText() {
  auto items = listFiles();
  String text = "";
  for (auto const& i : items) {
    text += std::get<0>(i);
    text += " (";
    text += std::get<1>(i);
    text += " bytes)\n";
  }
  return text;
}

void sendMqttStatus(const String& msg) {
#ifdef USING_MQTT
  mqttClient.sendStatus(msg);
#endif
}

void sendMqttLog(const String& msg) {
#ifdef USING_MQTT
  mqttClient.sendLog(msg);
#endif
}

////////// Command Handlers Begin //////////

void cmdReboot(const CommandParam& param = CommandParam::INVALID) {
  LOGN(F("[Core] Reboot now"));
  sendMqttLog("System will reboot now");
  compat::restart();
}

void cmdReset(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  pump.reset();
}

void cmdEnable(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  pump.setTimerEnabled(true);
}

void cmdDisable(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  pump.setTimerEnabled(false);
}

void cmdStart(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  pump.start();
}

void cmdStop(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  pump.stop();
}

void cmdClear(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  FileFS.remove("/serial.log");
  LOGN(F("Logs cleared"));
}

void cmdDelete(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  auto args = param.args;
  if (args.size() < 2) {
    return;
  }
  LOGF("cmdDelete file:%s\n", args[1]);
  auto fileName = String(args[1].c_str());
  SPIFFS.remove(fileName);
  String s = "File ";
  s += fileName;
  s += " deleted.";
  LOGN(s);
}

void cmdFiles(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  sendMqttStatus(getFilesText());
}

void cmdLogs(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  String logText = F("Go to http://");
  logText += WiFi.localIP().toString();
  logText += "/serial.log";
  sendMqttStatus(logText);
}

void cmdStatus(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  statusReport();
}

void cmdWiFi(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  String data = "";
  data += "Device: ";
  data += compat::getUDID();
  data += "\nMac: ";
  data += WiFi.macAddress();
  data += "\nIP: ";
  data += WiFi.localIP().toString();
  data += "\nSSID: ";
  data += WiFi.SSID();
  data += "\nUpTime: ";
  data += millis() / 1000;
  sendMqttStatus(data);
}

unsigned long parseLong(const string& extra) {
  if (extra.empty() || !ext::string::is_digits(extra)) {
    return 0;
  }
  return strtoul(extra.c_str(), nullptr, 0);
}

uint8_t parsePin(const string& extra) {
  if (extra.empty() || extra.length() > 2 || !ext::string::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

uint8_t parseValue(const string& extra) {
  if (extra.empty() || extra.length() > 1 || !ext::string::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

void cmdIOSet(const CommandParam& param = CommandParam::INVALID) {
  auto args = param.args;
  DLOG();
  if (args.size() < 2) {
    return;
  }
  uint8_t pin = parsePin(args[0]);
  if (pin == (uint8_t)0xff) {
    sendMqttLog(F("Invalid Pin"));
    return;
  }
  uint8_t value = parseValue(args[1]);
  if (value > (uint8_t)1) {
    sendMqttLog(F("Invalid Value"));
    return;
  }
}

void cmdHelp(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdHelp");
  sendMqttStatus(CommandManager.getHelpDoc());
}

void cmdNotFound(const CommandParam& param = CommandParam::INVALID) {
  DLOG();
  sendMqttLog(F("Send /help to see available commands"));
}

/////////// Command Handlers End ///////////

String getStatus() {
  DLOG();
  auto now = DateTime.getTime();
  auto ts = millis();
  auto cfg = pump.getConfig();
  auto st = pump.getStatus();
  String data = "";
  data += "Device: ";
  data += compat::getUDID();
  data += "\nBuild: ";
  data += __TIMESTAMP__;
  data += "\nPump Pin: ";
  data += pump.pin();
  data += "\nPump Status: ";
  data += (pump.pinValue() == HIGH) ? "Running" : "Idle";
  data += "\nMQTT Status: ";
#ifdef USING_MQTT
  data += mqttClient.isConnected() ? "Connected" : "Disconnected";
#else
  data += "Disabled";
#endif
  data += "\nWiFi IP: ";
  data += WiFi.localIP().toString();
  data += "\nRun Interval: ";
  data += humanTimeMs(cfg->interval);
  data += "\nRun Duration: ";
  data += humanTimeMs(cfg->duration);
#if defined(ESP8266)
  data += "\nFree Stack: ";
  data += ESP.getFreeContStack();
#endif
  data += "\nFree Heap: ";
  data += ESP.getFreeHeap();
  data += "\nLast Elapsed: ";
  data += st->lastElapsed / 1000;
  data += "s\nTotal Elapsed: ";
  data += st->totalElapsed / 1000;
  data += "s\nSystem Boot: ";
  data += formatDateTime(DateTime.getBootTime());
  if (st->lastStart > 0) {
    data += "\nLast Start: ";
    data += formatDateTime(now - (ts - st->lastStart) / 1000);
  } else {
    data += "\nLast Start: N/A";
  }
  if (st->lastStop > 0) {
    data += "\nLast Stop: ";
    data += formatDateTime(now - (ts - st->lastStop) / 1000);
  } else {
    data += "\nLast Stop: N/A";
  }
  data += "\nNext Start: ";
  if (!pump.isTimerEnabled()) {
    data += "N/A";
  } else {
    auto remains = st->lastStart + cfg->interval - ts;
    data += formatDateTime(now + remains / 1000);
  }
  data += "\nTimer Status: ";
  data += pump.isTimerEnabled() ? "Enabled" : "Disabled";
  return data;
}

void statusReport() { sendMqttStatus(getStatus()); }

void handleStart(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    cmdStart();
    // request->send(200, MIME_TEXT_PLAIN, "ok");
    request->redirect("/api/simple");

  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    cmdStop();
    // request->send(200, MIME_TEXT_PLAIN, "ok");
    request->redirect("/api/simple");
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleClear(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdClear();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleReboot(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdReboot();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleDisable(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdDisable();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleEnable(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdEnable();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleReset(AsyncWebServerRequest* request) {
  DLOG();
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdReset();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleRoot(AsyncWebServerRequest* request) {
  DLOG();
  request->send(SPIFFS, "/index.html");
  // request->redirect("/index.html");
}

void handleSerial(AsyncWebServerRequest* request) {
  DLOG();
  request->send(SPIFFS, "/serial.log", "text/plain");
  // request->redirect("/serial.log");
}

void handleIOSet(AsyncWebServerRequest* request) {
  DLOG();
  vector<string> args;
  for (size_t i = 0U; i < request->params(); i++) {
    auto p = request->getParam(i);
    LOGF("handleIOSet %s=%s\n", p->name().c_str(), p->value().c_str());
    string arg(p->value().c_str());
    arg += "=";
    arg += p->value().c_str();
    args.push_back(arg);
  }
  if (args.size() > 1) {
    const CommandParam param{"ioset", args};
    cmdIOSet(param);
  }
}

void handleHelp(AsyncWebServerRequest* request) {
  DLOG();
  request->send(200, MIME_TEXT_PLAIN, CommandManager.getHelpDoc());
}

void handleControl(AsyncWebServerRequest* request) {
  DLOG();
  String text = "";
  for (size_t i = 0U; i < request->params(); i++) {
    auto p = request->getParam(i);
    LOGF("handleControl %s=%s\n", p->name().c_str(), p->value().c_str());
    if (p->name() == "args") {
      text = p->value();
      break;
    }
  }
  LOGN("handleControl " + text);
  if (text.length() > 2) {
    string s(text.c_str());
    handleCommand(CommandParam::from(s));
  }
  request->send(200, MIME_TEXT_PLAIN, "ok");
}

void setupPump() {
  DLOG();
  ULOGN("[App] setupPump at PIN 13");
  // default pin nodemcu D5, nodemcu-32s 13
  pump.begin({"pump", 13, RUN_INTERVAL_DEFAULT, RUN_DURATION_DEFAULT});
  pump.setCallback([](const RelayEvent evt, int reason) {
    switch (evt) {
      case RelayEvent::Started: {
        digitalWrite(led, HIGH);
        String msg = F("[Core] Pump Started");
        LOGN(F("[Core] Pump Started"));
        // sendMqttStatus(msg);
      } break;
      case RelayEvent::Stopped: {
        digitalWrite(led, LOW);
        String msg = F("[Core] Pump Stopped");
        LOGN(F("[Core] Pump Stopped"));
        // sendMqttStatus(msg);
      } break;
      case RelayEvent::Enabled: {
        LOGN(F("[Core] Pump enabled"));
      } break;
      case RelayEvent::Disabled: {
        LOGN(F("[Core] Pump disabled"));
      } break;
      case RelayEvent::ConfigChanged: {
        LOGN(F("[Core] Config changed"));
      } break;
      default:
        break;
    }
  });
}

void handleCommand(const CommandParam& param) {
  DLOG();
  auto processFunc = [param] {
    yield();
    LOGN("[CMD] handleCommand");
    // LOGN(param.toString().c_str());
    if (!CommandManager.handle(param)) {
      LOGN("[CMD] Unknown command");
    }
  };
  Timer.setTimeout(5, processFunc, "handleCommand");
}

void setupCommands() {
  DLOG();
  //   CommandManager.setDefaultHandler(cmdNotFound);
  CommandManager.addCommand("clear", "clear current log", cmdClear);
  CommandManager.addCommand("reboot", "device reboot", cmdReboot);
  CommandManager.addCommand("on", "enable timers", cmdEnable);
  CommandManager.addCommand("enable", "enable timers", cmdEnable);
  CommandManager.addCommand("off", "disable timers", cmdDisable);
  CommandManager.addCommand("disable", "disable timers", cmdDisable);
  CommandManager.addCommand("reset", "reset timers", cmdReset);
  CommandManager.addCommand("start", "start device at pin", cmdStart);
  CommandManager.addCommand("stop", "stop device at pin", cmdStop);
  CommandManager.addCommand("status", "get device pin status", cmdStatus);
  CommandManager.addCommand("wifi", "get wifi status", cmdWiFi);
  CommandManager.addCommand("logs", "show device logs", cmdLogs);
  CommandManager.addCommand("delete", "delete the file", cmdDelete);
  CommandManager.addCommand("files", "show device files", cmdFiles);
  CommandManager.addCommand("ioset", "gpio set [pin] [value]", cmdIOSet);
  CommandManager.addCommand("list", "show commands", cmdHelp);
  CommandManager.addCommand("help", "show help", cmdHelp);
}

void setupApi() {
  webServer.setup([](std::shared_ptr<AsyncWebServer> server) {
    api.setup(server);
    server->on("/help", handleHelp);
  });
}

void setupApp() {
  setupCommands();
  setupPump();
}