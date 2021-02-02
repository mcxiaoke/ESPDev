#include <build.h>
#include <ALogger.h>
#include <Arduino.h>
#include <compat.h>
#include <net.h>
#include <utils.h>
#include <ArduinoTimer.h>
#include <ESPUpdateServer.h>
#include <FileServer.h>

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

constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";

constexpr const char *ssid = WIFI_SSID;
constexpr const char *password = WIFI_PASS;

constexpr const char *buildTime = "x";
constexpr const char *buildRev = "x";
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

void notFound(AsyncWebServerRequest *request)
{
  if (!FileServer::handle(request))
  {
    LOGN("handleNotFound " + request->url());
    String data = F("ERROR: NOT FOUND\nURI: ");
    data += request->url();
    data += "\n";
    request->send(404, MIME_TEXT_PLAIN, data);
  }
}

void setupServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/plain", "Hello, world");
  });
  server.onNotFound(notFound);
  otaUpdate.setup(&server);
  server.on("/gpio", handleGPIO);
  server.begin();
}
// ==== WebServer end   ====
// ========================================

size_t debugLog(const String text)
{
  return fileLog(text);
}

void checkDate()
{
  if (!DateTime.isTimeValid())
  {
    if (WiFi.isConnected())
    {
      LOGN("[System] checkDate");
      DateTime.setTimeZone(8);
      DateTime.begin();
    }
  }
}

void checkWiFi()
{
  //   LOGF("[WiFi] checkWiFi at %lus\n", millis() / 1000);
  if (!WiFi.isConnected())
  {
    WiFi.reconnect();
    debugLog(F("WiFi Reconnect"));
  }
}

void reboot()
{
  ESP.restart();
}

#if defined(ESP8266)
WiFiEventHandler h1, h2;
#elif defined(ESP32)
wifi_event_id_t h0, h1, h2;
#endif

void setupTimers()
{
  LOGN("[Init] setupTimers");
  timerReset = millis();
  aTimer.setInterval(10 * 60 * 1000L, checkWiFi, "checkWiFi");
}

void blinkLED()
{
  LOG(".");
  int v = digitalRead(led);
  digitalWrite(led, v == LOW ? HIGH : LOW);
}

void setupWiFi()
{
  LOGN("[WiFi] setupWiFi");

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  WiFi.mode(WIFI_STA);
  compat::setHostname(getUDID().c_str());
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  LOG("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    // LOG("..");
    // WiFi.reconnect();
    blinkLED();
    delay(500);
  }
  wifiInitialized = true;
  LOGN("\n[WiFi] Connected.");
  digitalWrite(led, HIGH);
}

void setupDate()
{
  LOGN("[Init] setupDate");
  if (WiFi.isConnected())
  {
    DateTime.setTimeZone(8);
    DateTime.begin();
  }
}

void setup(void)
{
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

void loop(void)
{
  aTimer.run();
#if defined(ESP8266)
  MDNS.update();
#endif
}

void showRequest(AsyncWebServerRequest *request)
{
  Serial.printf("[REQUEST]: %s %s\n", request->methodToString(), request->url().c_str());
  Serial.printf("[REMOTE IP]: %s\n", request->client()->remoteIP().toString().c_str());

  /***
  //List all collected headers
  int headers = request->headers();
  int i;
  for (i = 0; i < headers; i++)
  {
    AsyncWebHeader *h = request->getHeader(i);
    Serial.printf("HEADER [%s]: %s\n", h->name().c_str(), h->value().c_str());
  }
  ***/

  //List all parameters
  int params = request->params();
  for (int i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    if (p->isFile())
    { //p->isPost() is also true
      Serial.printf("FILE [%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    }
    else if (p->isPost())
    {
      Serial.printf("POST [%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
    else
    {
      Serial.printf("GET [%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
}

void handleGPIO(AsyncWebServerRequest *r)
{
  showRequest(r);
  // gpio/digital/
  // gpio/analog/
  // gpio/mode/
  String path = r->url();
  LOGN("handleGPIO path:",path);
  auto pathSegs = ext::string::split(string(r->url().c_str()),"/");
  LOGN("handleGPIO pathSegs:", ext::string::join(pathSegs,"/"));
  r->send(200,MIME_TEXT_PLAIN,"OK");
}