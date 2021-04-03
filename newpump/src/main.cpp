#include <RelayUnit.h>
#include <build.h>
#include <deps.h>
#include <rest.h>

using std::string;
// https://stackoverflow.com/questions/5287566/constexpr-and-deprecated-conversion-warning

constexpr const char* ssid = WIFI_SSID;
constexpr const char* password = WIFI_PASS;

constexpr const char* mqttServer = MQTT_SERVER;
constexpr const char* mqttUser = MQTT_USER;
constexpr const char* mqttPass = MQTT_PASS;
constexpr int mqttPort = MQTT_PORT;

constexpr const char* buildTime = APP_BUILD;
constexpr const char* buildRev = APP_REVISION;
constexpr int led = LED_BUILTIN;

#ifdef DEBUG
#define RUN_INTERVAL_DEFAULT 10 * 60 * 1000UL
#define RUN_DURATION_DEFAULT 18 * 1000UL
#else
#define RUN_INTERVAL_DEFAULT 24 * 3600 * 1000UL
#define RUN_DURATION_DEFAULT 60 * 1000UL
#endif

unsigned long timerReset = 0;
unsigned long startMillis = 0;
int mqttTimerId = -1;
constexpr const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";

AsyncWebServer server(80);
RelayUnit pump;
RestApi api(pump);
ESPUpdateServer otaUpdate(true);
#ifdef USING_MQTT
MqttManager mqttMgr(mqttServer, mqttPort, mqttUser, mqttPass);
#endif

void setupTimers(bool);
void checkWiFi();
String getStatus();
void statusReport();
void handleCommand(const CommandParam& param);
void sendMqttStatus(const String& msg);
void sendMqttLog(const String& msg);
bool mqttConnected();

String getFilesHtml() {
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

////////// MQTT Handlers Begin //////////

size_t debugLog(const String text) {
#ifndef DEBUG
#ifdef USING_MQTT
  sendMqttLog(text);
#endif
#endif
  return fileLog(text);
}

void setupMqtt() {
#ifdef USING_MQTT
  mqttMgr.begin(handleCommand);
#else
  debugLog("MQTT Disabled");
#endif
}

void mqttLoop() {
#ifdef USING_MQTT
  mqttMgr.loop();
#endif
}

void checkMqtt() {
#ifdef USING_MQTT
  //   LOGN("checkMqtt");
  mqttMgr.check();
#endif
}

void mqttTimer() {
#ifdef USING_MQTT
  Timer.setInterval((MQTT_KEEPALIVE * 10 - 5) * 1000L, checkMqtt, "checkMqtt");
#endif
}

void sendMqttStatus(const String& msg) {
#ifdef USING_MQTT
  mqttMgr.sendStatus(msg);
#ifdef USING_BLYNK
  terminal.println(msg);
  terminal.flush();
#endif
#endif
}

void sendMqttLog(const String& msg) {
#ifdef USING_MQTT
  mqttMgr.sendLog(msg);
#endif
}

bool mqttConnected() {
#ifdef USING_MQTT
  return mqttMgr.isConnected();
#else
  return false;
#endif
}
/////////// MQTT Handlers End ///////////

////////// Command Handlers Begin //////////

void cmdReboot(const CommandParam& param = CommandParam::INVALID) {
  debugLog(F("[Core] Reboot now"));
  sendMqttLog("System will reboot now");
  Timer.setTimeout(
      1000, []() { ESP.restart(); }, "cmdReboot");
}

void cmdReset(const CommandParam& param = CommandParam::INVALID) {
  pump.reset();
}

void cmdEnable(const CommandParam& param = CommandParam::INVALID) {
  pump.setTimerEnabled(true);
}

void cmdDisable(const CommandParam& param = CommandParam::INVALID) {
  pump.setTimerEnabled(false);
}

void cmdStart(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdStart");
  pump.start();
}

void cmdStop(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdStop");
  pump.stop();
}

void cmdClear(const CommandParam& param = CommandParam::INVALID) {
  SPIFFS.remove(logFileName());
  debugLog(F("Logs cleared"));
}

void cmdDelete(const CommandParam& param = CommandParam::INVALID) {
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
  debugLog(s);
}

void cmdFiles(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdFiles");
  sendMqttStatus(getFilesText());
}

void cmdLogs(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdLogs");
  String logText = F("Go to http://");
  logText += WiFi.localIP().toString();
  logText += logFileName();
  sendMqttStatus(logText);
}

void cmdStatus(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdStatus");
  statusReport();
}

void cmdWiFi(const CommandParam& param = CommandParam::INVALID) {
  LOGN("cmdWiFi");
  String data = "";
  data += "Device: ";
  data += getUDID();
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
  LOGN("cmdIOSet");
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
  LOGN("cmdNotFound");
  sendMqttLog(F("Send /help to see available commands"));
}

/////////// Command Handlers End ///////////

void checkWiFi() {
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
}

String getStatus() {
  auto ts = millis();
  auto cfg = pump.getConfig();
  auto st = pump.getStatus();
  String data = "";
  data += "Device: ";
  data += getUDID();
  data += "\nVersion: ";
  data += buildTime;
  data += "-";
  data += buildRev;
  data += "\nPump Pin: ";
  data += pump.pin();
  data += "\nPump Status: ";
  data += (pump.pinValue() == HIGH) ? "Running" : "Idle";
  data += "\nMQTT Status: ";
#ifdef USING_MQTT
  data += mqttMgr.isConnected() ? "Connected" : "Disconnected";
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
  data += formatDateTime(getTimestamp() - ts / 1000);
  if (st->lastStart > 0) {
    data += "\nLast Start: ";
    data += formatDateTime(getTimestamp() - (ts - st->lastStart) / 1000);
  } else {
    data += "\nLast Start: N/A";
  }
  if (st->lastStop > 0) {
    data += "\nLast Stop: ";
    data += formatDateTime(getTimestamp() - (ts - st->lastStop) / 1000);
  } else {
    data += "\nLast Stop: N/A";
  }
  data += "\nNext Start: ";
  if (!pump.isTimerEnabled()) {
    data += "N/A";
  } else {
    auto remains = st->lastStart + cfg->interval - ts;
    data += formatDateTime(getTimestamp() + remains / 1000);
  }
  data += "\nTimer Status: ";
  data += pump.isTimerEnabled() ? "Enabled" : "Disabled";
  data += "\nTimer Reset: ";
  data += formatDateTime(getTimestamp() - (ts - timerReset) / 1000);
  return data;
}

void statusReport() { sendMqttStatus(getStatus()); }

void handleFiles(AsyncWebServerRequest* request) {
  request->send(200, MIME_TEXT_HTML, getFilesHtml());
}

void handleLogs(AsyncWebServerRequest* request) {
  request->send(SPIFFS, logFileName(), MIME_TEXT_PLAIN);
}

void handleStart(AsyncWebServerRequest* request) {
  LOGN("handleStart");
  if (request->hasParam("do", false)) {
    cmdStart();
    // request->send(200, MIME_TEXT_PLAIN, "ok");
    request->redirect("/api/simple");

  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop(AsyncWebServerRequest* request) {
  LOGN("handleStop");
  if (request->hasParam("do", false)) {
    cmdStop();
    // request->send(200, MIME_TEXT_PLAIN, "ok");
    request->redirect("/api/simple");
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleClear(AsyncWebServerRequest* request) {
  LOGN("handleClear");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdClear();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleReboot(AsyncWebServerRequest* request) {
  LOGN("handleReboot");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdReboot();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleDisable(AsyncWebServerRequest* request) {
  LOGN("handleDisable");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdDisable();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleEnable(AsyncWebServerRequest* request) {
  LOGN("handleEnable");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdEnable();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleReset(AsyncWebServerRequest* request) {
  LOGN("handleReset");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdReset();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleSerial(AsyncWebServerRequest* request) {
  request->redirect("/serial.log");
}

void handleIOSet(AsyncWebServerRequest* request) {
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
  request->send(200, MIME_TEXT_PLAIN, CommandManager.getHelpDoc());
}

void handleWiFiGotIP() {
  LOGF("[WiFi] Connected to %s IP: %s\n", ssid, WiFi.localIP().toString());
  UDPSerial.setup();
}

void handleWiFiLost() {
  // WiFi.reconnect();
}

#if defined(ESP8266)
WiFiEventHandler h0, h1, h2;
#elif defined(ESP32)
wifi_event_id_t h0, h1, h2;
#endif

void setupWiFi() {
  LOGN("setupWiFi");
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  compat::setHostname(getHostName().c_str());

#if defined(ESP8266)
  h0 = WiFi.onStationModeConnected(
      [](const WiFiEventStationModeConnected& event) {
        // LOGN("[WiFi] Connected");
      });
  h1 = WiFi.onStationModeGotIP(
      [](const WiFiEventStationModeGotIP& event) { handleWiFiGotIP(); });
  h2 = WiFi.onStationModeDisconnected(
      [](const WiFiEventStationModeDisconnected& event) { handleWiFiLost(); });
#elif defined(ESP32)
  h1 = WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) { handleWiFiGotIP(); },
      WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  h2 = WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) { handleWiFiLost(); },
      WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#endif

  WiFi.begin(ssid, password);
  LOGF("[WiFi] ssid:%s, pass:%s\n", ssid, password);
  LOGN("[WiFi] Connecting...");
  auto startMs = millis();
  // setup wifi timeout 120 seconds
  while (!WiFi.isConnected() && (millis() - startMs) < 120 * 1000L) {
    delay(1000);
    if (millis() / 1000 % 5 == 0) {
      LOGN("[WiFi] Waiting connection...");
    }
  }
  if (!WiFi.isConnected()) {
    LOGN(F("[WiFi] Connect failed, will restart"));
    cmdReboot();
  }
  startMillis = millis();
  LOGF("[WiFi] Setup connection using %lus.\n", startMillis / 1000);
}

void setupDate() {
  LOGN("setupDate");
  if (WiFi.isConnected()) {
    DateTime.setServer("ntp.aliyun.com");
    DateTime.setTimeZone("CST-8");
    DateTime.begin();
  }
  if (!DateTime.isTimeValid()) {
    LOGN(F("[Date] NTP Sync failed."));
  } else {
    LOGF("[Date] NTP Sync using %lus.\n", (millis() - startMillis) / 1000);
    LOGF("[Date] Now %s\n", DateTime.toString());
  }
  startMillis = millis();
}

void setupApi() { api.setup(&server); }

void setupUpdate() {
  LOGN("setupUpdate");
  otaUpdate.setup(&server);
}

void handleControl(AsyncWebServerRequest* request) {
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

void handleNotFound(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
    return;
  }
  LOGN("handleNotFound " + request->url());
  if (request->url().startsWith("/api/")) {
    DynamicJsonDocument doc(128);
    doc["code"] = 404;
    doc["msg"] = "resource not found";
    doc["uri"] = request->url();
    String s = "";
    serializeJson(doc, s);
    request->send(404, JSON_MIMETYPE, s);
    return;
  }
  if (!FileServer::handle(request)) {
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += request->url();
    data += "\n";
    request->send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer() {
  LOGN("setupServer");
  if (MDNS.begin(getHostName().c_str())) {
    LOGN(F("[Server] MDNS responder started"));
  }
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.on("/serial", handleSerial);
  server.on("/files", handleFiles);
  server.on("/logs", handleLogs);
  server.on("/help", handleHelp);
  server.onNotFound(handleNotFound);
  setupApi();
  setupUpdate();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, POST, PUT, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Server] HTTP server started"));
}

void setupPump() {
  // default pin nodemcu D5, nodemcu-32s 13
  pump.begin({"pump", 13, RUN_INTERVAL_DEFAULT, RUN_DURATION_DEFAULT});
  pump.setCallback([](const RelayEvent evt, int reason) {
    switch (evt) {
      case RelayEvent::Started: {
        digitalWrite(led, HIGH);
        String msg = F("[Core] Pump Started");
        debugLog(msg);
        // sendMqttStatus(msg);
      } break;
      case RelayEvent::Stopped: {
        digitalWrite(led, LOW);
        String msg = F("[Core] Pump Stopped");
        debugLog(msg);
        // sendMqttStatus(msg);
      } break;
      case RelayEvent::Enabled: {
        debugLog(F("[Core] Pump enabled"));
      } break;
      case RelayEvent::Disabled: {
        debugLog(F("[Core] Pump disabled"));
      } break;
      case RelayEvent::ConfigChanged: {
        debugLog(F("[Core] Config changed"));
      } break;
      default:
        break;
    }
  });
}

void udpReport() {
  if (!WiFi.isConnected()) {
    return;
  }
  String s = getHostName();
  s += " Online Uptime: ";
  s += humanTimeMs(millis());
  UDPSerial.println(s);
}

void setupTimers(bool reset) {
  // LOGN("setupTimers");
  if (reset) {
    Timer.reset();
  }
  timerReset = millis();
  Timer.setInterval(5 * 60 * 1000L, checkWiFi, "checkWiFi");
  Timer.setTimeout(48 * 60 * 60 * 1000L, compat::restart, "reboot");
  Timer.setInterval(30 * 1000L, udpReport, "udp_report");
  mqttTimer();
}

void setupCommands() {
  // LOGN("setupCommands");
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

void checkModules() {
#ifndef USING_MQTT
  LOGN("[MQTT] MQTT Disabled");
#endif
#ifndef EANBLE_LOGGING
  LOGN("[Debug] Logging Disabled");
#endif
#ifdef DEBUG
  LOGN("[Debug] Debug Mode");
#endif
}

void setup(void) {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println();
  Serial.println("====== BOOT:BEGIN ======");
  fsCheck();
  FileSerial.setup();
  delay(200);
  debugLog("--------------------");
  setupWiFi();
  setupDate();
  debugLog("======@@@ Booting Begin @@@======");
  pinMode(led, OUTPUT);
  setupServer();
  setupMqtt();
  setupCommands();
  setupTimers(false);
  setupPump();
  checkModules();
  debugLog("[Core] System started at " + timeString());
  String info = "[Core] ";
  info += buildTime;
  info += "-";
  info += buildRev;
#ifdef DEBUG
  info += " (Debug Mode)";
#endif
  debugLog(info);
  debugLog("[Core] Sketch:" + ESP.getSketchMD5());
  LOGN("[Date] NTP:" + DateTime.toISOString());
  LOGN("[WiFi] IP:" + WiFi.localIP().toString());
  LOGN("[WiFi] Host:" + getHostName());
  LOGF("[Core] Booting using time: %ds\n", millis() / 1000);
  debugLog("======@@@ Booting Finished @@@======");
  Serial.println(ESP.getSketchMD5());
  Serial.println(dateTimeString());
  Serial.println(getHostName());
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP().toString());
  Serial.println("====== BOOT:FINISHED ======");
}

void loop(void) {
  otaUpdate.loop();
  UDPSerial.run();
  pump.run();
  Timer.run();
  mqttLoop();
#if defined(ESP8266)
  MDNS.update();
#endif
}

void handleCommand(const CommandParam& param) {
  auto processFunc = [param] {
    yield();
    LOG("[CMD] handleCommand ");
    // LOGN(param.toString().c_str());
    if (!CommandManager.handle(param)) {
      LOGN("[CMD] Unknown command");
    }
  };
  Timer.setTimeout(5, processFunc, "handleCommand");
}