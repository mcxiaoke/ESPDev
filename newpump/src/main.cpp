#include <ACommand.h>
#include <ALogger.h>
#include <Arduino.h>
#include <ArduinoTimer.h>
#include <ESPUpdateServer.h>
#include <FileServer.h>
#include <RelayUnit.h>
#include <build.h>
#include <compat.h>
#include <rest.h>
#ifdef USING_MQTT
#include <mqtt.h>
#endif
#ifdef USING_BLYNK
#if defined(ESP8266)
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
// #include <BlynkSimpleEsp8266_SSL.h>
#elif defined(ESP32)
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>
// #include <WiFiClientSecure.h>
// #include <BlynkSimpleEsp32_SSL.h>
#endif
#endif
#ifdef USING_DISPLAY
#include <display.h>
#endif
#include <net.h>
#include <utils.h>

using std::string;
// https://stackoverflow.com/questions/5287566/constexpr-and-deprecated-conversion-warning

constexpr const char* ssid = WIFI_SSID;
constexpr const char* password = WIFI_PASS;

constexpr const char* mqttServer = MQTT_SERVER;
constexpr const char* mqttUser = MQTT_USER;
constexpr const char* mqttPass = MQTT_PASS;
constexpr int mqttPort = MQTT_PORT;

constexpr const char* blynkAuth = BLYNK_AUTH;
constexpr const char* blynkHost = BLYNK_HOST;
constexpr int blynkPort = BLYNK_PORT;

constexpr const char* buildTime = APP_BUILD;
constexpr const char* buildRev = APP_REVISION;
constexpr int led = LED_BUILTIN;

#ifdef DEBUG
#define RUN_INTERVAL_DEFAULT 10 * 60 * 1000UL
#define RUN_DURATION_DEFAULT 18 * 1000UL
#define STATUS_INTERVAL_DEFAULT 60 * 60 * 1000UL
#else
#define RUN_INTERVAL_DEFAULT 24 * 3600 * 1000UL
#define RUN_DURATION_DEFAULT 120 * 1000UL
#define STATUS_INTERVAL_DEFAULT 12 * 60 * 60 * 1000UL
#endif

unsigned long statusInterval = STATUS_INTERVAL_DEFAULT;
unsigned long timerReset = 0;

bool wifiInitialized;
int wifiInitTimerId = -1;
int mqttTimerId = -1, statusTimerId = -1;
int displayTimerId = -1;
constexpr const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";

#ifdef USING_BLYNK
WidgetTerminal terminal(V20);
#endif

ArduinoTimer aTimer{"main"};
AsyncWebServer server(80);
RelayUnit pump;
RestApi api(pump);
#ifdef USING_DISPLAY
Display display;
#endif
ESPUpdateServer otaUpdate(true);
#ifdef USING_MQTT
MqttManager mqttMgr(mqttServer, mqttPort, mqttUser, mqttPass);
#endif

void setupTimers(bool);
void checkDate();
void checkBlynk();
void checkWiFi();
String getStatus();
void statusReport();
void handleCommand(const CommandParam& param);
void sendMqttStatus(const String& msg);
void sendMqttLog(const String& msg);
bool mqttConnected();

#ifdef USING_BLYNK
void blynkSyncPinValue();
void blynkSyncPinEnable();
#endif

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

void displayBooting() {
#ifdef USING_DISPLAY
  display.u8g2->setFont(u8g2_font_profont15_tf);
  display.u8g2->clearBuffer();
  display.u8g2->setCursor(16, 20);
  display.u8g2->print("Booting...");
  display.u8g2->setCursor(16, 50);
  display.u8g2->setFont(u8g2_font_profont29_tf);
  display.u8g2->print(getUDID());
  display.u8g2->sendBuffer();
  display.u8g2->setFont(u8g2_font_profont12_tf);
#endif
}

void updateDisplay() {
#ifdef USING_DISPLAY
  auto upSecs = millis() / 1000;

  String s1 = dateTimeString();
  String s2 = "";
  auto mod10 = upSecs % 16;
  if (mod10 < 8) {
    s2 += "WIFI:";
    s2 += WiFi.isConnected() ? "GOOD" : "BAD ";
    s2 += " MQTT:";
    s2 += mqttConnected() ? "GOOD" : "BAD ";
  } else {
    s2 += "PIN:";
    s2 += (pump.pinValue() == HIGH) ? "ON " : "OF";
    s2 += " UP:";
    s2 += monoTime(upSecs);
  }
  bool running = (pump.pinValue() == HIGH);
  String s3 = "";
  if (running) {
    s3 += "RUNNING";
  } else {
    if (!WiFi.isConnected()) {
      s3 += "NO WIFI";
    } else if (!DateTime.isTimeValid()) {
      s3 += "NO TIME";
    }
#ifdef USING_MQTT
    else if (!mqttConnected()) {
      s3 += "NO MQTT";
    }
#endif
    else {
      auto remains =
          pump.getStatus()->lastStart + pump.getConfig()->interval - millis();
      s3 += monoTimeMs(remains);
    }
  }
  //   yield();
  display.u8g2->clearBuffer();
  display.u8g2->setFont(u8g2_font_profont12_tf);
  display.u8g2->setCursor(2, 16);
  display.u8g2->print(s1);
  display.u8g2->setCursor(2, 30);
  display.u8g2->print(s2);
  display.u8g2->setCursor(2, 56);
  display.u8g2->setFont(u8g2_font_profont29_tf);
  display.u8g2->print(s3);
  display.u8g2->sendBuffer();
#endif
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
  aTimer.setInterval((MQTT_KEEPALIVE * 10 - 5) * 1000L, checkMqtt, "checkMqtt");
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
  aTimer.setTimeout(
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
  LOGN("cmdDelete");
  if (args.size() < 2) {
    return;
  }
  LOGN("cmdDelete file:", args[1]);
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

void cmdSettings(const CommandParam& param = CommandParam::INVALID) {
  auto args = param.args;
  LOGN("cmdSettings");
  if (args.size() < 1) {
    return;
  }
  uint8_t pin = 0;
  unsigned long interval = 0;
  unsigned long duration = 0;
  auto oldStatusInterval = statusInterval;
  // in seconds
  for (auto const& arg : args) {
    auto kv = ext::string::split(arg, "=");
    if (kv.size() == 2) {
      LOGF("Entry: %s=%s\n", kv[0].c_str(), kv[1].c_str());
      if (kv[0] == "pump_pin" || kv[0] == "pump") {
        pin = parsePin(kv[1]);
      } else if (kv[0] == "run_interval" || kv[0] == "runi") {
        interval = parseLong(kv[1]) * 1000UL;
      } else if (kv[0] == "run_duration" || kv[0] == "rund") {
        duration = parseLong(kv[1]) * 1000L;
      } else if (kv[0] == "status_interval" || kv[0] == "stsi") {
        statusInterval = parseLong(kv[1]) * 1000L;
      } else {
        String log = "[Settings] Invalid Entry: ";
        log += kv[0].c_str();
        debugLog(log);
        return;
      }
    }
  }

  LOGF("cmdSettings pin=%lu,interval=%lu,duration=%lu\n", pin, interval,
       duration);

  if (pin == 0 && interval == 0 && duration == 0) {
    debugLog("[Settings] Invalid Value");
    return;
  }

  int changed = pump.updateConfig({"newPump", pin, interval, duration});
  if (changed > 0) {
    debugLog("[Settings] Config changed");
  }
  if (statusInterval != oldStatusInterval) {
    debugLog("[Settings] Changed, reset timers");
    setupTimers(true);
  }
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

void cmdIOSetHigh(const CommandParam& param = CommandParam::INVALID) {
  auto args = param.args;
  LOGN("cmdIOSetHigh");
  if (args.size() < 1) {
    return;
  }
  uint8_t pin = parsePin(args[0]);
  if (pin > (uint8_t)0x80) {
    sendMqttLog("Invalid Pin");
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  String msg = F("cmdIOSetHigh for Pin ");
  msg += pin;
  debugLog(msg);
}

void cmdIOSetLow(const CommandParam& param = CommandParam::INVALID) {
  auto args = param.args;
  LOGN("cmdIOSetLow");
  if (args.size() < 1) {
    return;
  }
  uint8_t pin = parsePin(args[0]);
  if (pin > (uint8_t)0x80) {
    sendMqttLog("Invalid Pin");
    return;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  String msg = F("cmdIOSetLow for Pin ");
  msg += pin;
  debugLog(msg);
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

void checkDate() {
  if (!DateTime.isTimeValid()) {
    if (WiFi.isConnected()) {
      LOGN("[System] checkDate");
      DateTime.begin();
    }
  }
}

void checkBlynk() {
#ifdef USING_BLYNK
  if (!Blynk.connected()) {
    Blynk.connect();
  }
#endif
}

void checkWiFi() {
  //   LOGF("[WiFi] checkWiFi at %lus\n", millis() / 1000);
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
  data += "\nStatus Interval: ";
  data += humanTimeMs(statusInterval);
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

void statusReport() {
  LOGN("statusReport");
  sendMqttStatus(getStatus());
}

void handleFiles(AsyncWebServerRequest* request) {
  LOGN("handleFiles");
  request->send(200, MIME_TEXT_HTML, getFilesHtml());
}

void handleLogs(AsyncWebServerRequest* request) {
  LOGN("handleLogs");
  //   File file = SPIFFS.open(logFileName(), "r");
  request->send(SPIFFS, logFileName(), MIME_TEXT_PLAIN);
}

void handleStart(AsyncWebServerRequest* request) {
  LOGN("handleStart");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdStart();
  } else {
    request->send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop(AsyncWebServerRequest* request) {
  LOGN("handleStop");
  if (request->hasParam("do", false)) {
    request->send(200, MIME_TEXT_PLAIN, "ok");
    cmdStop();
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

void reboot() { ESP.restart(); }

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
  LOGN("handleRoot");
  request->redirect("/index.html");
  showESP();
}

void handleSettings(AsyncWebServerRequest* request) {
  vector<string> args;
  for (size_t i = 0U; i < request->params(); i++) {
    auto p = request->getParam(i);
    LOGF("handleSettings %s=%s\n", p->name().c_str(), p->value().c_str());
    string arg(p->value().c_str());
    arg += "=";
    arg += p->value().c_str();
    args.push_back(arg);
  }
  if (args.size() > 0) {
    const CommandParam param{"settings", args};
    cmdSettings(param);
  }
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
  if (!WiFi.isConnected()) {
    LOG("+++");
    return;
  }
  LOGN("[WiFi] Connected to", ssid, ",IP:", WiFi.localIP().toString());
  if (!wifiInitialized) {
    // on on setup stage
    wifiInitialized = true;
    LOGN("[WiFi] Initialized");
    aTimer.setTimeout(
        100,
        []() {
          LOGN("[WiFi] Check services");
          checkDate();
          checkMqtt();
          checkBlynk();
        },
        "wifiAfter");
  }
  if (wifiInitTimerId >= 0) {
    aTimer.deleteTimer(wifiInitTimerId);
    wifiInitTimerId = -1;
  }
}

void handleWiFiLost() { LOG("---"); }

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
  compat::setHostname(getUDID().c_str());

#if defined(ESP8266)
  h0 = WiFi.onStationModeConnected(
      [](const WiFiEventStationModeConnected& event) { LOG("///"); });
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
  PLOGF("[WiFi] ssid:%s, pass:%s\n", ssid, password);
  LOG("[WiFi] Connecting");
  auto startMs = millis();
  // setup wifi timeout 300 seconds
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 300 * 1000L) {
    LOG(".");
    delay(1000);
    if (millis() / 1000 % 10 == 0) {
      WiFi.reconnect();
      LOGF("=%d=", millis() / 1000);
    }
  }
  if (!WiFi.isConnected()) {
    PLOGN(F("[WiFi] Connect failed, will restart"));
    // WiFi.reconnect();
    // wifiInitTimerId = aTimer.setTimer(10 * 1000L, checkWiFi, 15, "wifiInit");
    ESP.restart();
  }
  wifiInitialized = true;
  LOGF("[WiFi] Setup connection using %lus.\n", millis() / 1000);
  PLOGF("[WiFi] IP:%s\n", WiFi.localIP().toString());
}

void setupDate() {
  LOGN("setupDate");
  if (WiFi.isConnected()) {
    DateTime.setServer("ntp.aliyun.com");
    DateTime.setTimeZone("CST-8");
    DateTime.begin();
    PLOGF("[Date] %s\n", DateTime.toISOString());
  }
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
    LOGN("handleNotFound " + request->url());
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += request->url();
    data += "\n";
    request->send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer() {
  LOGN("setupServer");
  if (MDNS.begin(getUDID().c_str())) {
    LOGN(F("[Server] MDNS responder started"));
  }
  time_t now = DateTime.getBootTime();
  struct tm* timeinfo;
  timeinfo = localtime(&now);
  server.serveStatic("/", SPIFFS, "/")
      .setDefaultFile("index.html")
      .setLastModified(timeinfo);
  //   server.on("/", handleRoot);
  server.on("/files", handleFiles);
  server.on("/logs", handleFiles);
  server.on("/help", handleHelp);
  server.onNotFound(handleNotFound);
  setupApi();
  setupUpdate();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  server.begin();
  MDNS.addService("http", "tcp", 80);
  PLOGN(F("[Server] HTTP server started"));
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
#ifdef USING_BLYNK
        blynkSyncPinValue();
#endif
      } break;
      case RelayEvent::Stopped: {
        digitalWrite(led, LOW);
        String msg = F("[Core] Pump Stopped");
        debugLog(msg);
        // sendMqttStatus(msg);
#ifdef USING_BLYNK
        blynkSyncPinValue();
#endif
      } break;
      case RelayEvent::Enabled: {
        debugLog(F("[Core] Pump enabled"));
#ifdef USING_BLYNK
        blynkSyncPinEnable();
#endif
      } break;
      case RelayEvent::Disabled: {
        debugLog(F("[Core] Pump disabled"));
#ifdef USING_BLYNK
        blynkSyncPinEnable();
#endif
      } break;
      case RelayEvent::ConfigChanged: {
        debugLog(F("[Core] Config changed"));
#ifdef USING_BLYNK
        blynkSyncPinEnable();
#endif
      } break;
      default:
        break;
    }
  });
}

void setupTimers(bool reset) {
  LOGN("setupTimers");
  if (reset) {
    aTimer.reset();
  }
  timerReset = millis();
  displayTimerId = aTimer.setInterval(1000, updateDisplay, "updateDisplay");
  aTimer.setInterval(5 * 60 * 1000L, checkWiFi, "checkWiFi");
  aTimer.setInterval(statusInterval, statusReport, "statusReport");
  aTimer.setTimeout(48 * 60 * 60 * 1000L, reboot, "reboot");
  mqttTimer();
}

void setupCommands() {
  LOGN("setupCommands");
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
  CommandManager.addCommand("online", "check device online", cmdWiFi);
  CommandManager.addCommand("logs", "show device logs", cmdLogs);
  CommandManager.addCommand("delete", "delete the file", cmdDelete);
  CommandManager.addCommand("files", "show device files", cmdFiles);
  CommandManager.addCommand("settings", "settings k1=v1 k2=v2", cmdSettings);
  CommandManager.addCommand("ioset", "gpio set [pin] [value]", cmdIOSet);
  CommandManager.addCommand("ioset1", "gpio set 1 for [pin]", cmdIOSetHigh);
  CommandManager.addCommand("ioset0", "gpio set 0 for [pin]", cmdIOSetLow);
  CommandManager.addCommand("list", "show commands", cmdHelp);
  CommandManager.addCommand("help", "show help", cmdHelp);
}

void setupDisplay() {
#ifdef USING_DISPLAY
  display.begin();
  displayBooting();
#endif
}

void setupBlynk() {
#ifdef USING_BLYNK
  //   Blynk.config(blynkAuth, blynkHost, blynkPort);
  Blynk.config(blynkAuth);
#endif
}

void checkModules() {
#ifndef USING_MQTT
  PLOGN("[MQTT] MQTT Disabled");
#endif
#ifndef EANBLE_LOGGING
  PLOGN("[Debug] Logging Disabled");
#endif
#ifndef USING_BLYNK
  PLOGN("[Blynk] Blynk Disabled");
#endif
#ifdef DEBUG
  PLOGN("[Debug] Debug Mode");
#else
  PLOGN("[Debug] Release Mode");
#endif
}

void setup(void) {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  showESP();
  fsCheck();
  delay(1000);
  setupDisplay();
  LOGN();
  PLOGN("======Booting Begin======");
  setupWiFi();
  setupDate();
  setupTimers(false);
  setupCommands();
  setupServer();
  setupMqtt();
  setupPump();
  setupBlynk();
  showESP();
  checkModules();
  debugLog(F("[Core] System started."));
#ifdef DEBUG
  fileLog(F("[Core] Debug Mode"));
#endif
  String info = "[Core] Ver:";
  info += buildTime;
  info += "-";
  info += buildRev;
  debugLog(info);
  PLOGN(info);
  PLOGN("======Booting Finished======");
}

void loop(void) {
  pump.run();
  aTimer.run();
  mqttLoop();
#if defined(ESP8266)
  MDNS.update();
#endif
#ifdef USING_BLYNK
  Blynk.run();
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
  aTimer.setTimeout(5, processFunc, "handleCommand");
}

#ifdef USING_BLYNK

void blynkSyncPinValue() { Blynk.virtualWrite(V1, pump.pinValue()); }
void blynkSyncPinEnable() {
  Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
}

void blynkSync() {
  Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
  Blynk.virtualWrite(V1, pump.pinValue());
  terminal.clear();
}

BLYNK_CONNECTED() {
  LOGN("BLYNK_CONNECTED");
  blynkSync();
}

BLYNK_APP_CONNECTED() { LOGN("BLYNK_APP_CONNECTED"); }

BLYNK_APP_DISCONNECTED() { LOGN("BLYNK_APP_DISCONNECTED"); }

BLYNK_DISCONNECTED() { LOGN("BLYNK_DISCONNECTED"); }

BLYNK_READ(V0) {
  // builtin led
  Blynk.virtualWrite(V0, digitalRead(led));
}

BLYNK_WRITE(V0) {
  // builtin led
  int value = param.asInt();
  LOGF("BLYNK_WRITE V0=%d\n", value);
  if (value == 0) {
    digitalWrite(led, LOW);
  } else if (value == 1) {
    digitalWrite(led, HIGH);
  }
}

BLYNK_READ(V1) {
  // pump switch
  Blynk.virtualWrite(V1, pump.pinValue());
}

BLYNK_WRITE(V1) {
  // pump switch
  int value = param.asInt();
  LOGF("BLYNK_WRITE V1=%d\n", value);
  if (value == LOW) {
    cmdStop();
  } else if (value == HIGH) {
    cmdStart();
  }
}

BLYNK_WRITE(V2) {
  // check status
  cmdStatus();
  terminal.flush();
}

#endif
