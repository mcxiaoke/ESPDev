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

void setupMqtt() {
#ifdef USING_MQTT
  mqttMgr.begin(handleCommand);
#else
  LOGN("MQTT Disabled");
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
  LOGN(F("[Core] Reboot now"));
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
  LOGN(F("Logs cleared"));
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
  LOGN(s);
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
    LOGN(F("[WiFi] WiFi Reconnecting"));
  } else {
    // ULOG("[WiFi] Connection is OK!");
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

void handleRoot(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/index.html");
  // request->redirect("/index.html");
}

void handleSerial(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/serial.log", "text/plain");
  // request->redirect("/serial.log");
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

bool setupWiFi() {
  Serial.println("setupWiFi");
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
  LOGF("[Setup] Connecting to WiFi:%s (%s)\n", ssid, password);
  auto startMs = millis();
  // setup wifi timeout 60 seconds
  while (!WiFi.isConnected() && (millis() - startMs) < 60 * 1000L) {
    delay(1000);
    if (millis() / 1000 % 10 == 0) {
      LOGF("[Setup] WiFi Connecting... (%d)\n", WiFi.status());
    }
  }
  if (!WiFi.isConnected()) {
    LOGN(F("[Setup] WiFi connect failed, will reboot"));
  } else {
    startMillis = millis();
    LOGF("[Setup] WiFi setup using %lus.\n", startMillis / 1000);
  }
  return WiFi.isConnected();
}

bool setupDate() {
  Serial.println("setupDate");
  if (!WiFi.isConnected()) {
    return false;
  }
  LOGN("[Setup] setupDate using ntp server1");
  DateTime.setTimeZone("CST-8");
  DateTime.setServer("cn.ntp.org.cn");
  DateTime.begin(10 * 1000L);

  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server2");
    DateTime.setServer("time.pool.aliyun.com");
    DateTime.begin(10 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server3");
    DateTime.setServer("ntp.ntsc.ac.cn");
    DateTime.begin(10 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN(F("[Setup] Time sync failed, will reboot"));
  } else {
    LOGF("[Setup] Time sync using %lus.\n", (millis() - startMillis) / 1000);
    startMillis = millis();
  }
  return DateTime.isTimeValid();
}

void setupApi() {
  Serial.println("setupApi");
  api.setup(&server);
}

void setupUpdate() {
  Serial.println("setupUpdate");
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
  ULOG("handleNotFound " + request->url());
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
  Serial.println("setupServer");
  if (MDNS.begin(getHostName().c_str())) {
    LOGN(F("[Setup] MDNS responder started"));
  }
  server.on("/", handleRoot);
  server.on("/serial", handleSerial);
  server.on("/files", handleFiles);
  server.on("/logs", handleLogs);
  server.on("/help", handleHelp);
  server.onNotFound(handleNotFound);
  setupApi();
  setupUpdate();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
  //                                      "GET, POST, PUT, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Setup] HTTP server started"));
}

void setupPump() {
  Serial.println("setupPump at PIN 13");
  // default pin nodemcu D5, nodemcu-32s 13
  pump.begin({"pump", 13, RUN_INTERVAL_DEFAULT, RUN_DURATION_DEFAULT});
  pump.setCallback([](const RelayEvent evt, int reason) {
    switch (evt) {
      case RelayEvent::Started: {
        digitalWrite(led, HIGH);
        String msg = F("[Core] Pump Started");
        LOGN(msg);
        // sendMqttStatus(msg);
      } break;
      case RelayEvent::Stopped: {
        digitalWrite(led, LOW);
        String msg = F("[Core] Pump Stopped");
        LOGN(msg);
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

void udpReport() {
  if (!WiFi.isConnected()) {
    return;
  }
  String s = getHostName();
  s += " Online (";
  s += humanTimeMs(millis());
  s += ")";
  UDPSerial.println(s);
}

void setupTimers(bool reset) {
  Serial.println("setupTimers");
  if (reset) {
    Timer.reset();
  }
  timerReset = millis();
  Timer.setInterval(5 * 60 * 1000L, checkWiFi, "checkWiFi");
  // Timer.setTimeout(48 * 60 * 60 * 1000L, compat::restart, "reboot");
  Timer.setInterval(60 * 1000L, udpReport, "udp_report");
}

void setupCommands() {
  Serial.println("setupCommands");
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

void setup(void) {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println();
  Serial.println("====== SETUP:BEGIN ======");
  Serial.println("FILE SYSTEM CHECK");
  fsCheck();
  FileSerial.setup();
  delay(200);
  bool done = setupWiFi();
  if (!done) {
    ESP.restart();
    return;
  }
  done = setupDate();
  if (!done) {
    ESP.restart();
    return;
  }
  LOGN("======@@@ Booting Begin @@@======");
  pinMode(led, OUTPUT);
  setupServer();
  setupMqtt();
  setupCommands();
  setupTimers(false);
  setupPump();
  LOGN("[Setup] System Started at " + timeString());
  String info = "[Setup] Version: ";
  info += buildTime;
  info += "-";
  info += buildRev;
#ifdef DEBUG
  info += " (Debug)";
#endif
  LOGN(info);
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGN("[Setup] WiFi IP: " + WiFi.localIP().toString());
  LOGN("[Setup] WiFi Host: " + getHostName());
  LOGF("[Setup] Booting using time: %ds\n", millis() / 1000);
  LOGN("======@@@ Booting Finished @@@======");
  Serial.println(ESP.getSketchMD5());
  Serial.println(dateTimeString());
  Serial.println(getHostName());
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP().toString());
  Serial.println("====== SETUP:FINISHED ======");
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
    LOGN("[CMD] handleCommand");
    // LOGN(param.toString().c_str());
    if (!CommandManager.handle(param)) {
      LOGN("[CMD] Unknown command");
    }
  };
  Timer.setTimeout(5, processFunc, "handleCommand");
}