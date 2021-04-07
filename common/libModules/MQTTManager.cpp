#include "MQTTManager.h"

static String stateToString(int state) {
  switch (state) {
    case MQTT_CONNECTION_TIMEOUT:
      return "MQTT_CONNECTION_TIMEOUT";
    case MQTT_CONNECTION_LOST:
      return "MQTT_CONNECTION_LOST";
    case MQTT_CONNECT_FAILED:
      return "MQTT_CONNECT_FAILED";
    case MQTT_DISCONNECTED:
      return "MQTT_DISCONNECTED";
    case MQTT_CONNECTED:
      return "MQTT_CONNECTED";
    case MQTT_CONNECT_BAD_PROTOCOL:
      return "MQTT_CONNECT_BAD_PROTOCOL";
    case MQTT_CONNECT_BAD_CLIENT_ID:
      return "MQTT_CONNECT_BAD_CLIENT_ID";
    case MQTT_CONNECT_UNAVAILABLE:
      return "MQTT_CONNECT_UNAVAILABLE";
    case MQTT_CONNECT_BAD_CREDENTIALS:
      return "MQTT_CONNECT_BAD_CREDENTIALS";
    case MQTT_CONNECT_UNAUTHORIZED:
      return "MQTT_CONNECT_UNAUTHORIZED";
    default:
      return "MQTTT_UNKNOWN";
      break;
  }
}

static string getDeviceId() {
  string mac(WiFi.macAddress().c_str());
  mac = ext::string::replace_all(mac, ":", "");
  return mac.substr(mac.length() / 2);
}

static String getOnlineMsg() {
  String msg = compat::getUDID();
  msg += "/Online ";
  msg += WiFi.localIP().toString();
  return msg;
}

static String getOfflineMsg() {
  String msg = compat::getUDID();
  msg += "/Offline ";
  msg += WiFi.localIP().toString();
  return msg;
}

// fix c++ linker undefined reference
// see https://stackoverflow.com/questions/16957458
constexpr unsigned int MQTTManager::COMMAND_MAX_LENGTH;
// constexpr const char* MQTTManager::TOPIC_DEVICE_CHECK = "device/check";
// constexpr const char* MQTTManager::TOPIC_DEVICE_ONLINE = "device/online";
// constexpr const char* MQTTManager::CMD_DEVICE_CHECK = "check";

MQTTManager::MQTTManager(const char* server, const int port,
                         const char* username, const char* password)
    : _server(server),
      _port(port),
      _username(username),
      _password(password),
      _client(WiFiClient()),
      _mqtt(new PubSubClient(_server, _port, _client)) {
  auto callback =
      std::bind(&MQTTManager::handleMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
  _mqtt->setCallback(callback);
  _mqtt->setBufferSize(1024);
  _mqtt->setKeepAlive(CUSTOM_MQTT_KEEPALIVE);
  _lastOnlineMs = 0;
  _lastOfflineMs = 0;
  _silentMode = false;
}

String MQTTManager::getUser() { return _username; }

String MQTTManager::getPass() { return _password; }

String MQTTManager::getClientId() { return compat::getUDID(); }

bool MQTTManager::isConnected() { return _mqtt->state() == MQTT_CONNECTED; }

string MQTTManager::getStatusTopic() {
  // device/%DEVICE%/status
  string topic("device/");
  topic += getDeviceId();
  topic += "/status";
  return topic;
}

string MQTTManager::getLogTopic() {
  // device/%DEVICE%/log
  string topic("device/");
  topic += getDeviceId();
  topic += "/logs";
  return topic;
}

string MQTTManager::getCmdTopic() {
  // device/%DEVICE%/cmd
  string topic("device/");
  topic += getDeviceId();
  topic += "/cmd";
  return topic;
}

string MQTTManager::getSerialTxTopic() {
  string topic("device/");
  topic += getDeviceId();
  topic += "/serial/tx";
  return topic;
}
string MQTTManager::getSerialRxTopic() {
  string topic("device/");
  topic += getDeviceId();
  topic += "/serial/rx";
  return topic;
}

void MQTTManager::sendStatus(const String& text) {
  if (!_mqtt->connected()) {
    // LOGN("[MQTT] Not connected, cannot send.");
    return;
  }
  bool ret = sendMessage(getStatusTopic().c_str(), text.c_str());
  if (ret) {
    // LOGF("[MQTT] status: [%s] (%d)\n", text.c_str(), text.length());
  } else {
    // LOGN("[MQTT] status sent failed.");
  }
}

void MQTTManager::sendLog(const String& text) {
  if (!_mqtt->connected()) {
    // LOGN("[MQTT] Not connected, cannot send.");
    return;
  }
  bool ret = sendMessage(getLogTopic().c_str(), text.c_str());
  if (ret) {
    // LOGF("[MQTT] log: [%s].\n", text.c_str());
  } else {
    // LOGN("[MQTT] log sent failed.");
  }
}

void MQTTManager::sendSerial(const String& text) {
  if (!_mqtt->connected()) {
    // LOGN("[MQTT] Not connected, cannot send.");
    return;
  }
  bool ret = sendMessage(getSerialTxTopic().c_str(), text.c_str());
  if (ret) {
    // LOGN("[MQTT] serial sent successful.");
  } else {
    // LOGN("[MQTT] serial sent failed.");
  }
}

void MQTTManager::connect() {
  // Loop until we're reconnected
  int maxRetries = 3;
  while (WiFi.isConnected() && !_mqtt->connected() && maxRetries-- > 0) {
    LOGF("[MQTT] Connecting to mqtt://%s\n", _server);
    // Attempt to connect
    // offline will message retain
    if (_mqtt->connect(getClientId().c_str(), getUser().c_str(),
                       getPass().c_str(), TOPIC_DEVICE_ONLINE, MQTTQOS0, true,
                       getOfflineMsg().c_str())) {
      LOGF("[MQTT] Connected to %s\n", _server);
      sendMessage(TOPIC_DEVICE_ONLINE, getOnlineMsg().c_str());
      sendOnline();
      initSubscribe();
      break;
    } else {
      LOGF("[MQTT] Connect failed:%s\n", stateToString(_mqtt->state()));
    }
  }
  checkStateChange();
}

void MQTTManager::check() {
  if (_silentMode) {
    return;
  }
  if (!_mqtt->connected()) {
    _lastOnlineMs = 0;
    _lastOfflineMs = millis();
    LOGN("[MQTT] Retry connect...");
    // Attempt to connect
    if (_mqtt->connect(getClientId().c_str(), getUser().c_str(),
                       getPass().c_str(), getStatusTopic().c_str(), MQTTQOS0,
                       true, "Offline")) {
      LOGN("[MQTT] Reconnected");
      sendOnline();
      initSubscribe();
    } else {
      LOGF("[MQTT] Reconnect failed:%d\n", stateToString(_mqtt->state()));
    }
  } else {
    ULOGN("[MQTT] Connection is OK!");
    // mqttPing();
  }
  checkStateChange();
}

void MQTTManager::setHandler(CMD_HANDLER_FUNC handler) { _handler = handler; }

bool MQTTManager::begin() {
  _lastCheckMs = millis();
  if (!_silentMode) {
    connect();
  } else {
    LOGN(F("[MQTT] Silent Mode, don't connect"));
  }
  // https://stackoverflow.com/questions/7582546
  // std::function<void(void)> f = std::bind(&Foo::doSomething, this);
  // using namespace std::placeholders;
  // std::function<void(int,int)> f = std::bind(&Foo::doSomethingArgs, this,
  // std::placeholders::_1, std::placeholders::_2);
  // auto func = std::bind(&MQTTManager::check, this);
  // Timer.setInterval((CUSTOM_MQTT_KEEPALIVE - 5) * 1000L, func, "mqtt_check");
  return true;
}

void MQTTManager::loop() {
  auto ms = millis();
  if (ms - _lastLoopCallMs > 500L) {
    _lastLoopCallMs = ms;
    _mqtt->loop();
    checkStateChange();
  }

  if (!WiFi.isConnected()) {
    return;
  }
  if (ms - _lastCheckMs > (CUSTOM_MQTT_KEEPALIVE * 10) * 1000L) {
    _lastCheckMs = ms;
    check();
  }
}

void MQTTManager::mute(bool silent) { _silentMode = silent; }

void MQTTManager::checkStateChange() {
  int newState = _mqtt->state();
  if (newState != _lastState) {
    _lastState = newState;
    LOGF("[MQTT] State changed to %s\n", stateToString(newState));
  }
}

void MQTTManager::handleMessage(const char* _topic, const uint8_t* _payload,
                                const unsigned int _length) {
  Serial.println(_topic);
  Serial.println((char*)_payload);
  Serial.flush();
  string topic("hello");
  string message("world");
  // string topic(_topic);
  // string message(_payload, _payload + _length);
  if (strcmp("test", _topic) == 0) {
    LOGF("[MQTT] Test message: %s\n", message.c_str());
    sendMessage("test/resp", message.c_str());
    return;
  }

  if (MQTTManager::TOPIC_DEVICE_CHECK == topic) {
    LOGN("[MQTT] Device check message.");
    sendMessage(TOPIC_DEVICE_ONLINE, getOnlineMsg().c_str());
    // hook for online command
    return;
    // topic = getCmdTopic();
    // message = "/online";
  }

  LOGF("[MQTT][%s] %s (%u)", topic.c_str(), message, _length);

  // replace newline for log print
  message = ext::string::replace_all(message, "\n", " ");
  ext::string::trim(message);
  if (topic != getCmdTopic()) {
    LOGN(F("[MQTT] Not a command"));
    // sendLog("What?");
    return;
  }
  if (!CommandParam::hasValidPrefix(message)) {
    LOGN(F("[MQTT] Command must start with /"));
    sendLog(F("Send /help to see available commands"));
    return;
  }
  if (_handler != nullptr) {
    _handler(CommandParam::from(message));
  }
}

bool MQTTManager::sendMessage(const char* topic, const char* payload,
                              boolean retained) {
  if (_silentMode) {
    // LOGN("[MQTT] Silent Mode, ignore sendMessage.");
    return false;
  } else {
    // LOGF("[MQTT] send [%s] to <%s>\n", payload, topic);
    return _mqtt->publish(topic, payload, retained);
  }
}

void MQTTManager::sendOnline() {
  // one online message per 30 minutes at most
  _lastOnlineMs = millis();
  // online message retain
  bool ret =
      sendMessage(getStatusTopic().c_str(), getOnlineMsg().c_str(), true);
  if (!ret) {
    // LOGN("[MQTT] online sent failed.");
  } else {
    // LOGN("[MQTT] online sent successfully.");
  }
}

void MQTTManager::initSubscribe() {
  _mqtt->subscribe("test");
  _mqtt->subscribe(TOPIC_DEVICE_CHECK);
  _mqtt->subscribe(getCmdTopic().c_str());
}