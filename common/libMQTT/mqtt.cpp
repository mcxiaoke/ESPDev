#include "mqtt.h"

static string getDeviceId() {
  string mac(WiFi.macAddress().c_str());
  mac = ext::string::replace_all(mac, ":", "");
  return mac.substr(mac.length() / 2);
}

static String getOnlineMsg() {
  String msg = getUDID();
  msg += "/Online ";
  msg += WiFi.localIP().toString();
  return msg;
}

static String getOfflineMsg() {
  String msg = getUDID();
  msg += "/Offline ";
  msg += WiFi.localIP().toString();
  return msg;
}

static void mqttFileLog(const String& text) {
  fileLog(text, logFileName(), true);
}

// fix c++ linker undefined reference
// see https://stackoverflow.com/questions/16957458
constexpr unsigned int MqttManager::COMMAND_MAX_LENGTH;
// constexpr const char* MqttManager::TOPIC_DEVICE_CHECK = "device/check";
// constexpr const char* MqttManager::TOPIC_DEVICE_ONLINE = "device/online";
// constexpr const char* MqttManager::CMD_DEVICE_CHECK = "check";

MqttManager::MqttManager(const char* server, const int port,
                         const char* username, const char* password)
    : _server(server),
      _port(port),
      _username(username),
      _password(password),
      _client(WiFiClient()),
      _mqtt(new PubSubClient(_server, _port, _client)) {
  auto callback =
      std::bind(&MqttManager::handleMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
  _mqtt->setCallback(callback);
  _mqtt->setBufferSize(1024);
  _mqtt->setKeepAlive(60);
  _lastOnlineMs = 0;
  _lastOfflineMs = 0;
  _silentMode = false;
}

String MqttManager::getUser() { return _username; }

String MqttManager::getPass() { return _password; }

String MqttManager::getClientId() { return getUDID(); }

bool MqttManager::isConnected() { return _mqtt->state() == MQTT_CONNECTED; }

string MqttManager::getStatusTopic() {
  // device/%DEVICE%/status
  string topic("device/");
  topic += getDeviceId();
  topic += "/status";
  return topic;
}

string MqttManager::getLogTopic() {
  // device/%DEVICE%/log
  string topic("device/");
  topic += getDeviceId();
  topic += "/logs";
  return topic;
}

string MqttManager::getCmdTopic() {
  // device/%DEVICE%/cmd
  string topic("device/");
  topic += getDeviceId();
  topic += "/cmd";
  return topic;
}

string MqttManager::getSerialTxTopic() {
  string topic("device/");
  topic += getDeviceId();
  topic += "/serial/tx";
  return topic;
}
string MqttManager::getSerialRxTopic() {
  string topic("device/");
  topic += getDeviceId();
  topic += "/serial/rx";
  return topic;
}

void MqttManager::sendStatus(const String& text) {
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

void MqttManager::sendLog(const String& text) {
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

void MqttManager::sendSerial(const String& text) {
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

void MqttManager::connect() {
  // Loop until we're reconnected
  int maxRetries = 3;
  while (WiFi.isConnected() && !_mqtt->connected() && maxRetries-- > 0) {
    LOGF("[MQTT] Connecting to mqtt://%s\n", _server);
    // Attempt to connect
    // offline will message retain
    if (_mqtt->connect(getClientId().c_str(), getUser().c_str(),
                       getPass().c_str(), TOPIC_DEVICE_ONLINE, MQTTQOS0, true,
                       getOfflineMsg().c_str())) {
      mqttFileLog("[MQTT] Connected to " + String(_server));
      sendMessage(TOPIC_DEVICE_ONLINE, getOnlineMsg().c_str());
      sendOnline();
      initSubscribe();
    } else {
      LOGF("[MQTT] Connect failed, rc=%d\n", _mqtt->state());
    }
  }
}

void MqttManager::check() {
  if (_silentMode) {
    return;
  }
  if (!WiFi.isConnected()) {
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
      mqttFileLog("[MQTT] Reconnected");
      sendOnline();
      initSubscribe();
    } else {
      String msg = "[MQTT] Reconnect failed, rc=";
      msg += _mqtt->state();
      mqttFileLog(msg);
    }
  } else {
    // LOGN("[MQTT] Connection is stable");
    // mqttPing();
  }
}

void MqttManager::begin(CMD_HANDLER_FUNC handler) {
  _handler = handler;
  if (!_silentMode) {
    connect();
  } else {
    LOGN(F("[MQTT] Silent Mode, don't connect"));
  }
}

void MqttManager::loop() {
  if (!_silentMode) {
    _mqtt->loop();
  }
}

void MqttManager::mute(bool silent) { _silentMode = silent; }

void MqttManager::handleStateChange(int state) {
  LOGF("[MQTT] State changed to %d\n", state);
}

void MqttManager::handleMessage(const char* _topic, const uint8_t* _payload,
                                const unsigned int _length) {
  yield();
  string topic(_topic);
  string message(_payload, _payload + _length);
  if (strcmp("test", _topic) == 0) {
    LOGF("[MQTT] Test message: %s\n", message.c_str());
    sendMessage("test/resp", message.c_str());
    return;
  }

  if (MqttManager::TOPIC_DEVICE_CHECK == topic) {
    LOGN("[MQTT] Device check message.");
    sendMessage(TOPIC_DEVICE_ONLINE, getOnlineMsg().c_str());
    // hook for online command
    return;
    // topic = getCmdTopic();
    // message = "/online";
  }

  mqttFileLog(ext::format::strFormat("[MQTT][%s] %s (%u)", topic.c_str(),
                                     message, _length)
                  .c_str());

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

bool MqttManager::sendMessage(const char* topic, const char* payload,
                              boolean retained) {
  if (_silentMode) {
    // LOGN("[MQTT] Silent Mode, ignore sendMessage.");
    return false;
  } else {
    // LOGF("[MQTT] send [%s] to <%s>\n", payload, topic);
    return _mqtt->publish(topic, payload, retained);
  }
}

void MqttManager::sendOnline() {
  // one online message per 30 minutes at most
  _lastOnlineMs = millis();
  // online message retain
  bool ret =
      sendMessage(getStatusTopic().c_str(), getOnlineMsg().c_str(), true);
  if (!ret) {
    LOGN("[MQTT] online sent failed.");
  } else {
    LOGN("[MQTT] online sent successfully.");
  }
}

void MqttManager::initSubscribe() {
  _mqtt->subscribe("test");
  _mqtt->subscribe(TOPIC_DEVICE_CHECK);
  _mqtt->subscribe(getCmdTopic().c_str());
}