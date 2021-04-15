#include <AWiFiManager.h>

static String stateToString(int state) {
  switch (state) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      return "WL_UNKNOWN";
      break;
  }
}

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
  _lastState = WL_IDLE_STATUS;
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
    if (millis() - lastConnectMs > 5000L) {
      lastConnectMs = millis();
      if (!WiFi.isConnected()) {
        LOGF("[WiFi] Connecting... (%ds)\n", millis() / 1000);
      }
    }
  }
  checkStatus();
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
    checkStatus();
  }
}

void AWiFiManagerClass::onConnected() {
  LOGF("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString());
  checkStatus();
  if (readyCallback) {
    readyCallback();
  }
}

void AWiFiManagerClass::onDisconnected() {
  checkStatus();
  if (lostCallback) {
    lostCallback();
  }
}

void AWiFiManagerClass::configEventHandler() {
#if defined(ESP8266)
  h0 = WiFi.onStationModeConnected(
      [this](const WiFiEventStationModeConnected& event) {
        // LOGN("[WiFi] Connected");
        checkStatus();
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
    LOGF("[WiFi] Reconnecting... %s",
         SafeMode.isEnabled() ? "(Safe Mode)" : "");
  } else {
    ULOGF("[WiFi] Network is OK! %s",
          SafeMode.isEnabled() ? "(Safe Mode)" : "");
  }
}

void AWiFiManagerClass::checkStatus() {
  int newState = WiFi.status();
  if (_lastState != newState) {
    _lastState = newState;
    LOGF("[WiFi] State changed to %s\n", stateToString(newState));
  }
}

AWiFiManagerClass AWiFi;