#include <Arduino.h>
#include <Wire.h>
#include "ext/string/string.hpp"
#include "ext/utility.hpp"
#include "libs/ESPUpdateServer.h"
#include "libs/FileServer.h"
#include "libs/SimpleTimer.h"
#include "libs/cmd.h"
#include "libs/compat.h"
#include "libs/config.h"
#include "libs/display.h"
#include "libs/net.h"
#include "libs/utils.h"
#ifdef USING_MQTT
#include "libs/mqtt.h"
#endif

using std::string;

#ifdef DEBUG_MODE
const unsigned long RUN_INTERVAL = 5 * 60 * 1000UL;
const unsigned long RUN_DURATION = 18 * 1000UL;
const unsigned long STATUS_INTERVAL = 10 * 60 * 1000UL;
#else
const unsigned long RUN_INTERVAL = 6 * 3600 * 1000UL;
const unsigned long RUN_DURATION = 15 * 1000UL;
const unsigned long STATUS_INTERVAL = 2 * 60 * 60 * 1000UL;
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const int led = LED_BUILTIN;
const int pump = 14;  // nodemcu D5

unsigned long lastStart = 0;
unsigned long lastStop = 0;
unsigned long lastSeconds = 0;
unsigned long totalSeconds = 0;

int runTimerId, mqttTimerId, statusTimerId;
int displayTimerId;
const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";
const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
const char MIME_TEXT_HTML[] PROGMEM = "text/html";

SimpleTimer timer;
#if defined(ESP8266)
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif
// ESPPrivateAccess serverAccess;

Display display;
ESPUpdateServer otaUpdate(true);
CommandManager cmdMgr;
#ifdef USING_MQTT
MqttManager mqttMgr(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);
#endif

void startPump();
void stopPump();
void checkPump();
void checkWiFi();
String getCommands();
String getStatus();
void statusReport();
void handleCommand(const CommandParam& param);
void sendMqttStatus(const String& msg);
void sendMqttLog(const String& msg);

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

void updateDisplay() {
  String s1 = "Timer: ";
  s1 += humanTimeMs(timer.getRemain(runTimerId));
  String s2 = "Pump Status: ";
  s2 += (digitalRead(pump) == HIGH) ? "Running" : "Idle";
  String s3 = "WiFi Status: ";
  s3 += WiFi.isConnected() ? "Good" : "Broken";
  String s4 = "MQTT Status: ";
  s4 += mqttMgr.isConnected() ? "Good" : "Broken";
  display.setText(s1, s2, s3, s4);
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
  timer.setInterval((MQTT_KEEPALIVE * 2 - 5) * 1000L, checkMqtt);
#endif
}

void sendMqttStatus(const String& msg) {
#ifdef USING_MQTT
  mqttMgr.sendStatus(msg);
#endif
}

void sendMqttLog(const String& msg) {
#ifdef USING_MQTT
  mqttMgr.sendLog(msg);
#endif
}
/////////// MQTT Handlers End ///////////

////////// Command Handlers Begin //////////

void cmdEnable(const CommandParam param = CommandParam::INVALID) {
  timer.enable(runTimerId);
  debugLog(F("Timer enabled"));
}
void cmdDisable(const CommandParam param = CommandParam::INVALID) {
  timer.disable(runTimerId);
  debugLog(F("Timer disabled"));
}

void cmdStart(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdStart");
  startPump();
}

void cmdStop(const CommandParam param = CommandParam::INVALID) {
  LOGN("cmdStop");
  stopPump();
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
  String wf = "Mac=";
  wf += WiFi.macAddress();
  wf += "\nIP=";
  wf += WiFi.localIP().toString();
  wf += "\nSSID=";
  wf += WiFi.SSID();
  wf += "\nStatus=";
  wf += WiFi.status();
  sendMqttStatus(wf);
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

void cmdIOSet(const CommandParam param = CommandParam::INVALID) {
  auto args = param.args;
  LOGF("cmdIOSet");
  if (args.size() < 3) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
  if (pin == (uint8_t)0xff) {
    sendMqttLog(F("Invalid Pin"));
    return;
  }
  uint8_t value = parseValue(args[2]);
  if (value > (uint8_t)1) {
    sendMqttLog(F("Invalid Value"));
    return;
  }
}

void cmdIOSetHigh(const CommandParam param = CommandParam::INVALID) {
  auto args = param.args;
  LOGF("cmdIOSetHigh");
  if (args.size() < 2) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
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
  LOGF("cmdIOSetLow");
  if (args.size() < 2) {
    return;
  }
  uint8_t pin = parsePin(args[1]);
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

void startPump() {
  LOGN("startPump");
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn) {
    return;
  }
  lastStart = millis();
  digitalWrite(pump, HIGH);
  digitalWrite(led, LOW);
  timer.setTimeout(RUN_DURATION, stopPump);
  String msg = F("Pump Started");
  debugLog(msg);
  msg += "\n";
  msg += getStatus();
  sendMqttStatus(msg);
}

void stopPump() {
  LOGN("stopPump");
  bool isOff = digitalRead(pump) == LOW;
  if (isOff) {
    return;
  }
  lastStop = millis();
  if (lastStart > 0) {
    lastSeconds = (lastStop - lastStart) / 1000;
    totalSeconds += lastSeconds;
  }
  digitalWrite(pump, LOW);
  digitalWrite(led, HIGH);
  String msg = F("Pump Stopped");
  debugLog(msg);
  msg += "\n";
  msg += getStatus();
  sendMqttStatus(msg);
}

void checkPump() {
  bool isOn = digitalRead(pump) == HIGH;
  if (isOn && lastStart > 0 && (millis() - lastStart) / 1000 >= RUN_DURATION) {
    debugLog(F("Pump stopped by watchdog"));
    cmdStop();
  }
}

void checkWiFi() {
  //   LOGN("checkWiFi");
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
}

String getStatus() {
  auto ts = millis();
  String data = "";
  data += "Device: ";
  data += getDevice();
  data += "\nTimer: ";
  data += timer.isEnabled(runTimerId) ? "Enabled" : "Disabled";
  data += "\nStatus: ";
  data += (digitalRead(pump) == HIGH) ? "Running" : "Idle";
  data += "\nMQTT: ";
#ifdef USING_MQTT
  data += mqttMgr.isConnected() ? "Connected" : "Disconnected";
#else
  data += "Disabled";
#endif
  data += "\nWiFi: ";
  data += WiFi.localIP().toString();
#if defined(ESP8266)
  data += "\nFree Stack: ";
  data += ESP.getFreeContStack();
#endif
  data += "\nFree Heap: ";
  data += ESP.getFreeHeap();
  data += "\nLast Elapsed: ";
  data += lastSeconds;
  data += "s\nTotal Elapsed: ";
  data += totalSeconds;
  data += "s\nSystem Boot: ";
  data += formatDateTime(getTimestamp() - ts / 1000);
  if (lastStart > 0) {
    data += "\nLast Start: ";
    data += formatDateTime(getTimestamp() - (ts - lastStart) / 1000);
  }
  if (lastStop > 0) {
    data += "\nLast Stop: ";
    data += formatDateTime(getTimestamp() - (ts - lastStop) / 1000);
  }
  data += "\nNext Start: ";
  data += formatDateTime(getTimestamp() + timer.getRemain(runTimerId) / 1000);
  return data;
}

void statusReport() {
  LOGN("statusReport");
  sendMqttStatus(getStatus());
}

void handleFiles() {
  LOGN("handleFiles");
  server.send(200, MIME_TEXT_HTML, getFilesHtml());
}

void handleLogs() {
  LOGN("handleLogs");
  File file = SPIFFS.open(logFileName(), "r");
  server.streamFile(file, MIME_TEXT_PLAIN);
}

void handleStart() {
  LOGN("handleStart");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStart();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleStop() {
  LOGN("handleStop");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdStop();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleClear() {
  LOGN("handleClear");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdClear();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void reboot() {
  ESP.restart();
}

void handleReboot() {
  LOGN("handleReboot");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    timer.setTimeout(1000, reboot);
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleDisable() {
  LOGN("handleDisable");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdDisable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleEnable() {
  LOGN("handleEnable");
  if (server.hasArg("do")) {
    server.send(200, MIME_TEXT_PLAIN, "ok");
    cmdEnable();
  } else {
    server.send(200, MIME_TEXT_PLAIN, "ignore");
  }
}

void handleRoot() {
  LOGN("handleRoot");
  server.send(200, MIME_TEXT_PLAIN, getStatus().c_str());
  showESP();
  //   String data = "text=";
  //   data += urlencode("Pump_Status_Report_");
  //   data += urlencode(getDevice());
  //   data += "&desp=";
  //   data += urlencode(getStatus());
  //   httpsPost(WX_REPORT_URL, data);
}

void handleIOSet() {
  for (auto i = 0; i < server.args(); i++) {
    LOGF("handleIOSet %s=%s\n", server.argName(i).c_str(),
         server.arg(i).c_str());
  }
  //   if (server.hasArg("pin") && server.hasArg("value")) {}
}

void setupWiFi() {
  LOGN("setupWiFi");
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  //   WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
#if defined(ESP8266)
//   WiFi.hostname(getDevice().c_str());
#elif defined(ESP32)
//   WiFi.setHostname(getDevice().c_str());
#endif
  WiFi.begin(ssid, password);
  Serial.print("WiFi Connecting");
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 30 * 1000L) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.isConnected()) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(led, HIGH);
  } else {
    Serial.println("WiFi connect failed");
  }
}

void setupDate() {
  LOGN("setupDate");
  setTimestamp();
}

void setupUpdate() {
  LOGN("setupUpdate");
  otaUpdate.setup(&server);
}

void handleControl() {
  String text = "";
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "args") {
      text = server.arg(i);
      break;
    }
  }
  LOGN("handleControl " + text);
  if (text.length() > 2) {
    string s(text.c_str());
    handleCommand(CommandParam::parseArgs(s));
  }
  server.send(200, MIME_TEXT_PLAIN, "ok");
}

void handleNotFound() {
  if (!FileServer::handle(&server)) {
    LOGN("handleNotFound " + server.uri());
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += server.uri();
    data += "\n";
    server.send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer() {
  LOGN("setupServer");
  if (MDNS.begin(getDevice().c_str())) {
    LOGN(F("[Server] MDNS responder started"));
  }
  server.on("/", handleRoot);
  server.on("/cmd", handleControl);
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
  server.onNotFound(handleNotFound);
  setupUpdate();
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGN(F("[Server] HTTP server started"));

  //   auto h = serverAccess.getLastHandler(server);
  //   while (h) {
  //     if (instanceof <RequestHandler>(h)) {
  //       // pass
  //     }
  //     h = h->next();
  //   }
}

void setupTimers() {
  LOGN("setupTimers");
  displayTimerId = timer.setInterval(1000, updateDisplay);
  runTimerId = timer.setInterval(RUN_INTERVAL, startPump);
  timer.setInterval(RUN_DURATION / 2 + 2000, checkPump);
  timer.setInterval(5 * 60 * 1000L, checkWiFi);
  timer.setInterval(STATUS_INTERVAL, statusReport);
  mqttTimer();
}

void setupCommands() {
  LOGN("setupCommands");
  cmdMgr.setDefaultHandler(cmdNotFound);
  cmdMgr.addCommand("on", "set timer on", cmdEnable);
  cmdMgr.addCommand("enable", "set timer on", cmdEnable);
  cmdMgr.addCommand("off", "set timer off", cmdDisable);
  cmdMgr.addCommand("disable", "set timer off", cmdDisable);
  cmdMgr.addCommand("start", "start pump", cmdStart);
  cmdMgr.addCommand("stop", "stop pump", cmdStop);
  cmdMgr.addCommand("status", "get pump status", cmdStatus);
  cmdMgr.addCommand("wifi", "get wifi status", cmdWiFi);
  cmdMgr.addCommand("logs", "show logs", cmdLogs);
  cmdMgr.addCommand("files", "show files", cmdFiles);
  cmdMgr.addCommand("ioset", "gpio set [pin] [value]", cmdIOSet);
  cmdMgr.addCommand("ioset1", "gpio set to 1 for [pin]", cmdIOSetHigh);
  cmdMgr.addCommand("ioset0", "gpio set to 0 for  [pin]", cmdIOSetLow);
  cmdMgr.addCommand("list", "show available commands", cmdHelp);
  cmdMgr.addCommand("help", "show help message", cmdHelp);
}

void setupDisplay() {
  display.begin();
  display.setText("Pump", "Booting...", "Wifi", "Connecting...");
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(115200);
  showESP();
  fsCheck();
  setupDisplay();
  delay(1000);
  setupWiFi();
  setupDate();
  setupServer();
  setupTimers();
  setupMqtt();
  setupCommands();
  showESP();
  debugLog(F("System is up and running"));
}

void loop(void) {
  mqttLoop();
  server.handleClient();
#if defined(ESP8266)
  MDNS.update();
#endif
  timer.run();
}

void handleCommand(const CommandParam& param) {
  auto processFunc = [param] {
    showESP();
    LOG("[CMD] handleCommand ");
    LOGN(param.toString().c_str());
    if (!cmdMgr.handle(param)) {
      LOGN("[CMD] Unknown command");
    }
    yield();
    showESP();
  };
  timer.setTimeout(5, processFunc);
}
