// #define BLYNK_PRINT Serial
#define BLYNK_NO_BUILTIN
#define BLYNK_NO_FLOAT

#include <Arduino.h>
#include "ext/string/string.hpp"
#include "libs/ArduinoTimer.h"
#include "libs/ESPUpdateServer.h"
#include "libs/FileServer.h"
#include "libs/RelayUnit.h"
#include "libs/cmd.h"
#include "libs/compat.h"
#include "libs/config.h"
#include "libs/display.h"
#include "libs/net.h"
#include "libs/rest.h"
#include "libs/utils.h"
#ifdef USING_MQTT
#include "libs/mqtt.h"
#endif

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

using std::string;

#define STR1(x) #x
#define STR(x) STR1(x)

#ifdef PIO_SRC_REV
#define BUILD_VERSION STR(PIO_SRC_REV)
#else
#define BUILD_VERSION "-"
#endif

#ifdef DEBUG_MODE
#define RUN_INTERVAL_DEFAULT 5 * 60 * 1000UL
#define RUN_DURATION_DEFAULT 18 * 1000UL
#define STATUS_INTERVAL_DEFAULT 10 * 60 * 1000UL
#else
#define RUN_INTERVAL_DEFAULT 8 * 3600 * 1000UL
#define RUN_DURATION_DEFAULT 15 * 1000UL
#define STATUS_INTERVAL_DEFAULT 2 * 60 * 60 * 1000UL
#endif

const char* buildVersion = BUILD_VERSION;
const char* ssid = STASSID;
const char* password = STAPSK;
const int led = LED_BUILTIN;

unsigned long statusInterval = STATUS_INTERVAL_DEFAULT;
unsigned long timerReset = 0;

bool wifiInitialized;
int wifiInitTimerId = -1;
int mqttTimerId = -1, statusTimerId = -1;
int displayTimerId = -1;
const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
const char MIME_TEXT_HTML[] PROGMEM = "text/html";

WidgetTerminal terminal(V20);
ArduinoTimer aTimer{"main"};
AsyncWebServer server(80);
RelayUnit pump;

Display display;
ESPUpdateServer otaUpdate(true);
CommandManager cmdMgr;
#ifdef USING_MQTT
MqttManager mqttMgr(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);
#endif

void setupTimers(bool);
void checkDate();
void checkBlynk();
void checkWiFi();
String getCommands();
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

void displayBooting() {
  display.u8g2->setFont(u8g2_font_profont15_tf);
  display.u8g2->clearBuffer();
  display.u8g2->setCursor(16, 20);
  display.u8g2->print("Booting...");
  display.u8g2->setCursor(16, 50);
  display.u8g2->setFont(u8g2_font_profont29_tf);
  display.u8g2->print(getDevice());
  display.u8g2->sendBuffer();
  display.u8g2->setFont(u8g2_font_profont12_tf);
}

void updateDisplay() {
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
    if (!hasValidTime()) {
      s3 += "NO TIME";
    } else if (!WiFi.isConnected()) {
      s3 += "NO WIFI";
    } else if (!mqttConnected()) {
      s3 += "NO MQTT";
    } else {
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
}

////////// MQTT Handlers Begin //////////

size_t debugLog(const String text) {
#ifdef USING_MQTT
  sendMqttLog(text);
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
  aTimer.setInterval((MQTT_KEEPALIVE * 2 - 5) * 1000L, checkMqtt, "checkMqtt");
#endif
}

void sendMqttStatus(const String& msg) {
#ifdef USING_MQTT
  mqttMgr.sendStatus(msg);
  terminal.println(msg);
  terminal.flush();
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

void cmdReboot(const CommandParam param = CommandParam::INVALID) {
  debugLog(F("Reboot now"));
  sendMqttLog("System will reboot now");
  aTimer.setTimeout(1000, []() { ESP.restart(); }, "cmdReboot");
}

void cmdEnable(const CommandParam param = CommandParam::INVALID) {
  pump.setEnabled(true);
}

void cmdDisable(const CommandParam param = CommandParam::INVALID) {
  pump.setEnabled(false);
}

void cmdStart(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdStart");
  pump.start();
}

void cmdStop(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdStop");
  pump.stop();
}

void cmdClear(const CommandParam param = CommandParam::INVALID) {
  SPIFFS.remove(logFileName());
  debugLog(F("Logs cleared"));
}

void cmdFiles(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdFiles");
  sendMqttStatus(getFilesText());
}

void cmdLogs(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdLogs");
  String logText = F("Go to http://");
  logText += WiFi.localIP().toString();
  logText += logFileName();
  sendMqttStatus(logText);
}

void cmdStatus(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdStatus");
  statusReport();
}

void cmdWiFi(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdWiFi");
  String data = "";
  data += "Device: ";
  data += getDevice();
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
  if (&extra == nullptr || extra.empty() || !extstring::is_digits(extra)) {
    return 0;
  }
  return strtoul(extra.c_str(), nullptr, 0);
}

uint8_t parsePin(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 2 ||
      !extstring::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

uint8_t parseValue(const string& extra) {
  if (&extra == nullptr || extra.empty() || extra.length() > 1 ||
      !extstring::is_digits(extra)) {
    return 0xff;
  }
  return atoi(extra.c_str());
}

void cmdSettings(const CommandParam param = CommandParam::INVALID) {
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
    auto kv = extstring::split(arg, "=");
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
        String log = "Invalid Entry: ";
        log += kv[0].c_str();
        sendMqttLog(log);
        return;
      }
    }
  }

  if (pin == 0 || interval == 0 || duration == 0) {
    debugLog("Invalid Setting Value");
    return;
  }

  int changed = pump.updateConfig({"newPump", pin, interval, duration});
  if (changed > 0) {
    sendMqttLog("Pump config changed");
  }
  if (statusInterval != oldStatusInterval) {
    sendMqttLog("Settings changed, reset timers");
    setupTimers(true);
  }
}

void cmdIOSet(const CommandParam param = CommandParam::INVALID) {
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

void cmdIOSetHigh(const CommandParam param = CommandParam::INVALID) {
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

void cmdIOSetLow(const CommandParam param = CommandParam::INVALID) {
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

void cmdHelp(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdHelp");
  sendMqttStatus(cmdMgr.getHelpDoc());
}

void cmdNotFound(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdNotFound");
  sendMqttLog(F("Send /help to see available commands"));
}

/////////// Command Handlers End ///////////

void checkDate() {
  if (!hasValidTime()) {
    if (WiFi.isConnected()) {
      LOGN("[System] checkDate");
      setTimestamp();
      if (hasValidTime()) {
        aTimer.setBootTime(getBootTime());
      }
    }
  }
}

void checkBlynk() {
  if (!Blynk.connected()) {
    Blynk.connect();
  }
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
  data += getDevice();
  data += "\nVersion: ";
  data += buildVersion;
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
  data += "\nTimer Status: ";
  data += pump.isEnabled() ? "Enabled" : "Disabled";
  data += "\nTimer Reset: ";
  data += formatDateTime(getTimestamp() - (ts - timerReset) / 1000);
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
  }
  if (st->lastStop > 0) {
    data += "\nLast Stop: ";
    data += formatDateTime(getTimestamp() - (ts - st->lastStop) / 1000);
  }
  data += "\nNext Start: ";
  auto remains = st->lastStart + cfg->interval - millis();
  data += formatDateTime(getTimestamp() + remains / 1000);
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

void reboot() {
  ESP.restart();
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

void handleRoot(AsyncWebServerRequest* request) {
  LOGN("handleRoot");
  request->send(200, MIME_TEXT_PLAIN, getStatus().c_str());
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

void handleWiFiGotIP() {
  LOG("[WiFi] Connected to ");
  LOG(ssid);
  LOG(", IP: ");
  LOGN(WiFi.localIP());
  aTimer.setTimeout(100,
                    []() {
                      checkDate();
                      checkMqtt();
                      checkBlynk();
                    },
                    "onWiFiGotIP");
  if (!wifiInitialized) {
    wifiInitialized = true;
    LOGN("[WiFi] Initialized");
  }
  if (wifiInitTimerId >= 0) {
    aTimer.deleteTimer(wifiInitTimerId);
    wifiInitTimerId = -1;
  }
}

void handleWiFiLost() {
  LOG("---");
}

#if defined(ESP8266)
WiFiEventHandler h1, h2;
#elif defined(ESP32)
wifi_event_id_t h1, h2;
#endif
void setupWiFi() {
  LOGN("setupWiFi");
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  //   WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
#if defined(ESP8266)
  WiFi.hostname(getDevice().c_str());
#elif defined(ESP32)
  WiFi.setHostname(getDevice().c_str());
#endif

#if defined(ESP8266)
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
  LOG("[WiFi] Connecting");
  auto startMs = millis();
  // setup wifi timeout 60 seconds
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 60 * 1000L) {
    LOG(".");
    delay(1000);
  }
  LOGN();
  if (!WiFi.isConnected()) {
    LOGN("[WiFi] Connect failed, will retry");
    WiFi.reconnect();
    wifiInitTimerId = aTimer.setTimer(10 * 1000L, checkWiFi, 15, "wifiInit");
  } else {
    LOGF("[WiFi] Setup connection using %lus.\n", millis() / 1000);
  }
}

void setupDate() {
  LOGN("setupDate");
  if (WiFi.isConnected()) {
    setTimestamp();
  }
}

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
    handleCommand(CommandParam::parseArgs(s));
  }
  request->send(200, MIME_TEXT_PLAIN, "ok");
}

void handleNotFound(AsyncWebServerRequest* request) {
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
  if (MDNS.begin(getDevice().c_str())) {
    LOGN(F("[Server] MDNS responder started"));
  }
  server.on("/", handleRoot);
  server.on("/cmd", handleControl);
  server.on("/settings", handleSettings);
  server.on("/api/status", handleRoot);
  server.on("/reboot", handleReboot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/clear", handleClear);
  server.on("/disable", handleDisable);
  server.on("/enable", handleEnable);
  server.on("/on", handleEnable);
  server.on("/off", handleDisable);
  server.on("/ioset", handleIOSet);
  server.on("/files", handleFiles);
  server.on("/logs", handleLogs);
  //   server.serveStatic("/www/", SPIFFS,
  //   "/www/").setCacheControl("max-age=600");
  server.onNotFound(handleNotFound);
  setupUpdate();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Server] HTTP server started"));
}

void setupPump() {
  // default pin nodemcu D5
  pump.begin({"pump", 14, RUN_INTERVAL_DEFAULT, RUN_DURATION_DEFAULT});
  pump.setCallback([](const RelayEvent evt, int reason) {
    switch (evt) {
      case RelayEvent::STARTED: {
        String msg = F("Pump Started");
        debugLog(msg);
        sendMqttStatus(msg);
        Blynk.virtualWrite(V1, pump.pinValue());
      } break;
      case RelayEvent::STOPPED: {
        digitalWrite(led, HIGH);
        String msg = F("Pump Stopped");
        debugLog(msg);
        sendMqttStatus(msg);
        Blynk.virtualWrite(V1, pump.pinValue());
      } break;
      case RelayEvent::ENABLED: {
        debugLog(F("Pump enabled"));
        Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
      } break;
      case RelayEvent::DISABLED: {
        debugLog(F("Pump disabled"));
        Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
      } break;
      case RelayEvent::RESET: {
        debugLog(F("Pump reset"));
        Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
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
  mqttTimer();
}

void setupCommands() {
  LOGN("setupCommands");
  cmdMgr.setDefaultHandler(cmdNotFound);
  cmdMgr.addCommand("reboot", "device reboot", cmdReboot);
  cmdMgr.addCommand("on", "enable timers", cmdEnable);
  cmdMgr.addCommand("enable", "enable timers", cmdEnable);
  cmdMgr.addCommand("off", "disable timers", cmdDisable);
  cmdMgr.addCommand("disable", "disable timers", cmdDisable);
  cmdMgr.addCommand("start", "start device at pin", cmdStart);
  cmdMgr.addCommand("stop", "stop device at pin", cmdStop);
  cmdMgr.addCommand("status", "get device pin status", cmdStatus);
  cmdMgr.addCommand("wifi", "get wifi status", cmdWiFi);
  cmdMgr.addCommand("online", "check device online", cmdWiFi);
  cmdMgr.addCommand("logs", "show device logs", cmdLogs);
  cmdMgr.addCommand("files", "show device files", cmdFiles);
  cmdMgr.addCommand("settings", "settings k1=v1 k2=v2", cmdSettings);
  cmdMgr.addCommand("ioset", "gpio set [pin] [value]", cmdIOSet);
  cmdMgr.addCommand("ioset1", "gpio set 1 for [pin]", cmdIOSetHigh);
  cmdMgr.addCommand("ioset0", "gpio set 0 for [pin]", cmdIOSetLow);
  cmdMgr.addCommand("list", "show commands", cmdHelp);
  cmdMgr.addCommand("help", "show help", cmdHelp);
}

void setupDisplay() {
  display.begin();
  displayBooting();
}

void setup(void) {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  showESP();
  fsCheck();
  setupDisplay();
  delay(1000);
  setupTimers(false);
  setupWiFi();
  setupDate();
  setupServer();
  setupMqtt();
  setupCommands();
  Blynk.config(BLYNK_AUTH, BLYNK_HOST, BLYNK_PORT);
  setupPump();
  showESP();
  debugLog(F("System is running"));
  LOGN(__TIME__);
}

void loop(void) {
  pump.run();
  aTimer.run();
  mqttLoop();
#if defined(ESP8266)
  MDNS.update();
#endif
  Blynk.run();
}

void handleCommand(const CommandParam& param) {
  auto processFunc = [param] {
    yield();
    LOG("[CMD] handleCommand ");
    LOGN(param.toString().c_str());
    if (!cmdMgr.handle(param)) {
      LOGN("[CMD] Unknown command");
    }
  };
  aTimer.setTimeout(5, processFunc, "handleCommand");
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

BLYNK_APP_CONNECTED() {
  LOGN("BLYNK_APP_CONNECTED");
}

BLYNK_APP_DISCONNECTED() {
  LOGN("BLYNK_APP_DISCONNECTED");
}

BLYNK_DISCONNECTED() {
  LOGN("BLYNK_DISCONNECTED");
}

BLYNK_READ(V0) {
  Blynk.virtualWrite(V0, pump.isEnabled() ? HIGH : LOW);
}

BLYNK_WRITE(V0) {
  // global timer switch
  int value = param.asInt();
  LOGF("BLYNK_WRITE V0=%d\n", value);
  if (value == 0) {
    cmdDisable();
  } else if (value == 1) {
    cmdEnable();
  }
}

BLYNK_READ(V1) {
  Blynk.virtualWrite(V1, pump.pinValue());
}

BLYNK_WRITE(V1) {
  // pump switch
  int value = param.asInt();
  LOGF("BLYNK_WRITE V1=%d\n", value);
  if (value == 0) {
    cmdStop();
  } else if (value == 1) {
    cmdStart();
  }
}

BLYNK_WRITE(V20) {
  const char* value = param.asStr();
  const auto cmd = CommandParam::parseArgs(value);
  handleCommand(cmd);
  terminal.flush();
}
