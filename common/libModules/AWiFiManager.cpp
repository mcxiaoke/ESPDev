#include <AWiFiManager.h>

AWiFiManagerClass::AWiFiManagerClass(WiFiMode_t _mode) : mode(_mode) {}

AWiFiManagerClass::~AWiFiManagerClass() {}

void AWiFiManagerClass::setTimeout(unsigned long _timeoutMs) {
  timeoutMs = _timeoutMs;
}

void AWiFiManagerClass::setCredentials(const char* _ssid,
                                       const char* _password) {
  ssid = _ssid;
  password = _password;
}

bool AWiFiManagerClass::begin() {
  WiFi.mode(mode);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  compat::setHostname(compat::getHostName().c_str());
  configEventHandler();
  WiFi.begin(ssid, password);
  LOGF("[WiFi] Connecting to %s (%s)\n", ssid, password);
  auto startMs = millis();
  // default timeout 60 seconds
  while (!WiFi.isConnected() && (millis() - startMs) < timeoutMs) {
    delay(500);
    if (millis() - lastConnectMs > 3000L) {
      lastConnectMs = millis();
      if (!WiFi.isConnected()) {
        LOGF("[WiFi] Connecting... (%ds)\n", millis() / 1000);
      }
    }
  }
  lastConnectMs = 0;
  if (!WiFi.isConnected()) {
    LOGN(F("[WiFi] Connect failed."));
  } else {
    LOGF("[WiFi] Setup using time: %lums.\n", millis());
  }
  return WiFi.isConnected();
}

void AWiFiManagerClass::loop() {
  auto now = millis();
  if (now - lastCheckMs > checkIntervalMs) {
    lastCheckMs = now;
    checkConnection();
  }
}

void AWiFiManagerClass::onConnected() {
  LOGF("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString());
  if (readyCallback) {
    readyCallback();
  }
}

void AWiFiManagerClass::onDisconnected() {
  if (lostCallback) {
    lostCallback();
  }
}

void AWiFiManagerClass::configEventHandler() {
#if defined(ESP8266)
  h0 = WiFi.onStationModeConnected(
      [](const WiFiEventStationModeConnected& event) {
        // LOGN("[WiFi] Connected");
      });
  h1 = WiFi.onStationModeGotIP(
      [this](const WiFiEventStationModeGotIP& event) { onConnected(); });
  h2 = WiFi.onStationModeDisconnected(
      [this](const WiFiEventStationModeDisconnected& event) {
        onDisconnected();
      });
#elif defined(ESP32)
  h1 = WiFi.onEvent(
      [this](WiFiEvent_t event, WiFiEventInfo_t info) { onConnected(); },
      WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  h2 = WiFi.onEvent(
      [this](WiFiEvent_t event, WiFiEventInfo_t info) { onDisconnected(); },
      WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#endif
}

void AWiFiManagerClass::checkConnection() {
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
    LOGN(F("[WiFi] Reconnecting..."));
  } else {
    ULOGN("[WiFi] Network is OK!");
  }
}

AWiFiManagerClass AWiFi;