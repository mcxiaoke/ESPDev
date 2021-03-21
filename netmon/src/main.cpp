#include <Arduino.h>
#include <ArduinoTimer.h>
#include <ESPUpdateServer.h>
#include <FileServer.h>
#include <compat.h>
#include <mqtt.h>
#include <net.h>
#include <time.h>
#include <tools.h>

#include "build.h"
#include "private.h"

#ifndef DEBUG_UPDATER
#define DEBUG_UPDATER Serial
#endif

struct LanDevice {
  const char* name;
  const char* info;
  const char* host;
  const uint16_t port;
  bool online;
  unsigned long lastOnlineMs;

  String toString() const {
    return String(name) + "/" + String(host) + ":" + String(port) + " " +
           (online ? "Online" : "Offline");
  }

  String msgTitle() {
    return "Device " + String(name) + (online ? " Online" : " Offline");
  }

  String msgDesp() { return "IP: " + String(host) + " (" + String(info) + ")"; }
};

constexpr int led = LED_BUILTIN;
constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";

LanDevice pump{"ESP_DF975D", "ESP Plant Watering Device", "192.168.1.116", 80};
LanDevice armn1{"Armbian_N1", "N1 Armbian Home Server", "192.168.1.165", 22};
LanDevice n54l{"N54L", "Windows 7 NAS Server", "192.168.1.110", 80};

WiFiClient client;
AsyncWebServer server(80);
ESPUpdateServer otaUpdate(true);

void blinkLED() {
  int v = digitalRead(led);
  digitalWrite(led, v == LOW ? HIGH : LOW);
}

void sendMessage(String title, String content = "") {
  String wxUrl = WX_REPORT_URL;
  wxUrl += "?title=";
  wxUrl += urlencode(title);
  if (content && content.length() > 0) {
    wxUrl += "&desp=";
    wxUrl += urlencode(content);
  }
  wifiHttpGet(wxUrl, client);
}

bool checkPortOpen(const LanDevice& device) {
  client.setTimeout(5000);
  client.connect(device.host, device.port);
  yield();
  bool online = client.connected();
  client.stop();
  return online;
}

void checkDevice(LanDevice& device) {
  bool online = checkPortOpen(device);
  if (online != device.online) {
    device.online = online;
    sendMessage(device.msgTitle(), device.msgDesp());
    fileLog("[Monitor] " + device.toString() + " at " + timeString());
  }
}

void checkAllPorts() {
  Timer.setTimeout(3000L, []() { checkDevice(pump); });
  Timer.setTimeout(100L, []() { checkDevice(armn1); });
  Timer.setTimeout(1500L, []() { checkDevice(n54l); });
}

void checkWiFi() {
  if (!compat::isWiFiConnected()) {
    WiFi.reconnect();
  }
}

void sendOnline() {
  sendMessage("LAN Monitor " + getUDID() + " Online",
              "IP: " + WiFi.localIP().toString());
}

void setupWiFi() {
  Serial.println("[WiFi] setupWiFi");
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  WiFi.mode(WIFI_STA);
  compat::setHostname(getUDID().c_str());
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("[WiFi] Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    blinkLED();
    delay(200);
    if (millis() > 120 * 1000L) {
      break;
    }
  }
  if (!compat::isWiFiConnected()) {
    compat::restart();
    return;
  }
  Serial.println("[WiFi] Connected.");
  Serial.println(WiFi.localIP().toString());
  digitalWrite(led, LOW);
  sendOnline();
}

void setupDate() {
  DateTime.setServer("ntp.aliyun.com");
  DateTime.setTimeZone("CST-8");
  DateTime.begin();
  Serial.println(DateTime.toISOString());
}

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
  html += "<p> You can delete file by add '?delete=1' to file url.<p>";
  return html;
}

void handleLogs(AsyncWebServerRequest* request) {
  LOGN("handleLogs");
  request->send(FileFS, logFileName(), MIME_TEXT_PLAIN);
}

void handleFiles(AsyncWebServerRequest* request) {
  request->send(200, MIME_TEXT_HTML, getFilesHtml());
}

void handleNotFound(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    request->send(200);
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

void handleRoot(AsyncWebServerRequest* request) {
  request->send(FileFS, "/index.html");
}

void setupServer() {
  otaUpdate.setup(&server);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  server.onNotFound(handleNotFound);
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest* request) {
    String content = "UDID: ";
    content += getUDID();
    content += "\nMD5: ";
    content += ESP.getSketchMD5();
    content += "\nTime:";
    content += DateTime.toString();
    request->send(200, "text/plain", content);
  });
  server.on("/files", handleFiles);
  server.on("/logs", handleLogs);
  server.on("/", handleRoot);
  server.begin();
  MDNS.begin(getHostName().c_str());
  MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  fsCheck();
  setupWiFi();
  setupDate();
  setupServer();
  // check wifi status every 10 minutes
  Timer.setInterval(10 * 60 * 1000L, checkWiFi, "checkWiFi");
  // check ports open every 5 minutes
  Timer.setInterval(5 * 60 * 1000L, checkAllPorts, "checkPorts");
  // send online message every 12 hours
  Timer.setInterval(12 * 60 * 60 * 1000L, sendOnline, "sendOnline");
  // trim log file every day
  Timer.setInterval(
      24 * 60 * 60 * 1000L, []() { trimLogFile(); }, "trim_log");
  // reboot device after 48 hours
  Timer.setTimeout(48 * 60 * 60 * 1000L, compat::restart, "reboot");
  delay(1000);
  checkAllPorts();
}

void loop() {
#if defined(ESP8266)
  MDNS.update();
#endif
  Timer.run();
  otaUpdate.loop();
}