#define DEBUG_UPDATER Serial

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

constexpr int led = LED_BUILTIN;
constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";

#define HOST_N1 "192.168.1.165"
#define PORT_N1 22
#define HOST_N54L "192.168.1.110"
#define PORT_N54L 80
#define HOST_PUMP "192.168.1.116"
#define PORT_PUMP 6053

bool n1Online;
bool n54lOnline;
bool pumpOnline;

WiFiClient client;
ArduinoTimer timer{"main"};
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

bool checkPortOpen(String host, int port) {
  client.connect(host, port);
  yield();
  // if (!client.connected()) {
  //   sendMessage("Device " + host + " Offline", "Offline IP: " + host);
  //   return false;
  // }
  bool ret = client.connected();
  client.stop();
  return ret;
}

void checkN1() {
  bool online = checkPortOpen(HOST_N1, PORT_N1);
  if (online != n1Online || !online) {
    n1Online = online;
    sendMessage("Device " + String(HOST_N1) + (online ? " Online" : " Offline"),
                "IP: " + String(HOST_N1));
  }
}

void checkN54L() {
  bool online = checkPortOpen(HOST_N54L, PORT_N54L);
  if (online != n54lOnline) {
    n54lOnline = online;
    sendMessage(
        "Device " + String(HOST_N54L) + (online ? " Online" : " Offline"),
        "IP: " + String(HOST_N54L));
  }
}

void checkPump() {
  bool online = checkPortOpen(HOST_PUMP, PORT_PUMP);
  if (online != pumpOnline || !online) {
    pumpOnline = online;
    sendMessage(
        "Device " + String(HOST_PUMP) + (online ? " Online" : " Offline"),
        "IP: " + String(HOST_PUMP));
  }
}

void checkAllPorts() {
  Serial.println("Checking lan network ports.");
  checkN1();
  checkN54L();
  checkPump();
}

void checkWiFi() {
  if (!compat::isWiFiConnected()) {
    WiFi.reconnect();
  }
}

void sendOnline() {
  sendMessage("Monitor " + getUDID() + " Online",
              "Online IP: " + WiFi.localIP().toString());
}

void reboot() { ESP.reset(); }

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
    ESP.reset();
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
  return html;
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
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
  MDNS.begin(getUDID());
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
  timer.setInterval(8 * 60 * 1000L, checkWiFi, "checkWiFi");
  // check ports open every 15 minutes
  timer.setInterval(5 * 60 * 1000L, checkAllPorts, "checkPorts");
  // send online message every 12 hours
  timer.setInterval(12 * 60 * 60 * 1000L, sendOnline, "sendOnline");
  // reboot device after 48 hours
  timer.setTimeout(48 * 60 * 60 * 1000L, reboot, "reboot");
  delay(1000);
  checkAllPorts();
}

void loop() {
#if defined(ESP8266)
  MDNS.update();
#endif
  timer.run();
  otaUpdate.loop();
}