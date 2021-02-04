#include <ALogger.h>
#include <Arduino.h>
#include <ArduinoTimer.h>
#include <ESPUpdateServer.h>
#include <FileServer.h>
#include <webio.h>
#include <build.h>
#include <compat.h>
#include <net.h>
#include <utils.h>

// espserver lib include begin
#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
// espserver lib include end

using std::string;
// https://stackoverflow.com/questions/5287566/constexpr-and-deprecated-conversion-warning
constexpr const char *ssid = WIFI_SSID;
constexpr const char *password = WIFI_PASS;
constexpr int led = LED_BUILTIN;

ArduinoTimer aTimer{"main"};
unsigned long timerReset = 0;
bool wifiInitialized;
int wifiInitTimerId = -1;

void setupTimers();
void setupDate();
void checkDate();
void checkWiFi();
int ledControl(String command);
void handleGPIO(AsyncWebServerRequest *r);

// ========================================
// ==== WebServer begin ====

AsyncWebServer server(80);
ESPUpdateServer otaUpdate(true);

void notFound(AsyncWebServerRequest *request) {
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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/plain", "Server on ESP32-"+getUDID());
  });
  server.onNotFound(notFound);
  otaUpdate.setup(&server);
  server.on("/gpio", handleWebIO);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}
// ==== WebServer end   ====
// ========================================

size_t debugLog(const String text) { return fileLog(text); }

void checkDate() {
  if (!DateTime.isTimeValid()) {
    if (WiFi.isConnected()) {
      LOGN("[System] checkDate");
      DateTime.setTimeZone(8);
      DateTime.begin();
    }
  }
}

void checkWiFi() {
  //   LOGF("[WiFi] checkWiFi at %lus\n", millis() / 1000);
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
}

void reboot() { ESP.restart(); }

#if defined(ESP8266)
WiFiEventHandler h1, h2;
#elif defined(ESP32)
wifi_event_id_t h0, h1, h2;
#endif

void setupTimers() {
  LOGN("[Init] setupTimers");
  timerReset = millis();
  aTimer.setInterval(10 * 60 * 1000L, checkWiFi, "checkWiFi");
}

void blinkLED() {
  LOG(".");
  int v = digitalRead(led);
  digitalWrite(led, v == LOW ? HIGH : LOW);
}

void setupWiFi() {
  LOGN("[WiFi] setupWiFi");

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  compat::setHostname(getUDID().c_str());
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  LOG("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    // LOG("..");
    // WiFi.reconnect();
    blinkLED();
    delay(500);
  }
  wifiInitialized = true;
  LOGN("\n[WiFi] Connected.");
  digitalWrite(led, HIGH);
}

void setupDate() {
  LOGN("[Init] setupDate");
  if (WiFi.isConnected()) {
    DateTime.setTimeZone(8);
    DateTime.begin();
  }
}

void setup(void) {
  Serial.begin(115200);
  fsCheck();
  delay(200);
  setupWiFi();
  setupDate();
  setupTimers();
  setupServer();
  debugLog(F("[Init] System started."));
#ifdef DEBUG
  fileLog(F("[Init] Debug Mode"));
#endif
  LOGN("[Init] Time: " + DateTime.toString());
  LOGN("[Init] IP: " + WiFi.localIP().toString());
  LOGN("[Init] Mac: " + WiFi.macAddress());
}

void loop(void) {
  aTimer.run();
#if defined(ESP8266)
  MDNS.update();
#endif
}

