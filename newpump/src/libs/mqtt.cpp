#include "mqtt.h"

static bool checkCommand(string message) {
  //   LOGN("checkCommand");
  static const char* CMD_PREFIX = "/#@!$%";
  return message.length() > 2 && strchr(CMD_PREFIX, message.at(0)) != nullptr;
}

static string getDeviceId() {
  string mac(WiFi.macAddress().c_str());
  mac = extstring::replace_all(mac, ":", "");
  return mac.substr(mac.length() / 2);
}

static void mqttFileLog(const String& text) {
  LOGN(text);
  fileLog(text, logFileName("mqtt"), true);
}

MqttManager::MqttManager(const char* server,
                         const int port,
                         const char* username,
                         const char* password)
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
}

String MqttManager::getUser() {
  return _username;
}

String MqttManager::getPass() {
  return _password;
}

String MqttManager::getClientId() {
  return getDevice();
}

bool MqttManager::isConnected() {
  return _mqtt->connected();
}

bool MqttManager::isCommand(const string& topic) {
  return topic == getCmdTopic();
}

string MqttManager::getStatusTopic() {
  // pump/%DEVICE%/status
  string topic("pump/");
  topic += getDeviceId();
  topic += "/status";
  return topic;
}

string MqttManager::getLogTopic() {
  // pump/%DEVICE%/log
  string topic("pump/");
  topic += getDeviceId();
  topic += "/logs";
  return topic;
}

string MqttManager::getCmdTopic() {
  // pump/%DEVICE%/cmd
  string topic("pump/");
  topic += getDeviceId();
  topic += "/cmd";
  return topic;
}

void MqttManager::sendStatus(const String& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = _mqtt->publish(getStatusTopic().c_str(), text.c_str());
  if (ret) {
    // LOGN("[MQTT] mqtt message sent successful.");
  } else {
    LOGN("[MQTT] mqtt status sent failed.");
  }
}

void MqttManager::sendStatus2(const std::string& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = _mqtt->publish(getStatusTopic().c_str(), text.c_str());
  if (ret) {
    LOGN("[MQTT] mqtt status sent successful.");
  } else {
    LOGN("[MQTT] mqtt status sent failed.");
  }
}

void MqttManager::sendLog(const String& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = _mqtt->publish(getLogTopic().c_str(), text.c_str());
  if (ret) {
    LOGN("[MQTT] mqtt log sent successful.");
  } else {
    LOGN("[MQTT] mqtt log sent failed.");
  }
}

void MqttManager::sendLog2(const std::string& text) {
  //   LOGF("[MQTT] send message: [%s]\n", text.c_str());
  bool ret = _mqtt->publish(getLogTopic().c_str(), text.c_str());
  if (!ret) {
    LOGN("[MQTT] mqtt resp sent failed.");
  }
}

void MqttManager::connect() {
  // Loop until we're reconnected
  int maxRetries = 10;
  while (!_mqtt->connected() && maxRetries-- > 0) {
    LOGF("[MQTT] Connecting to mqtt://%s:%s@%s\n", getUser().c_str(),
         getPass().c_str(), _server);
    // Attempt to connect
    // offline will message retain
    if (_mqtt->connect(getClientId().c_str(), getUser().c_str(),
                       getPass().c_str(), getStatusTopic().c_str(), MQTTQOS2,
                       true, "Offline")) {
      String msg = "[MQTT] Connected to ";
      msg += _server;
      msg += " as ";
      msg += getClientId();
      mqttFileLog(msg);
      sendOnline();
      _mqtt->subscribe("test");
      _mqtt->subscribe(getCmdTopic().c_str());

    } else {
      LOGN("[MQTT] Connect failed, rc=");
      LOG(_mqtt->state());
      LOGN("[MQTT] Connect try again in 5 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

void MqttManager::check() {
  // Loop until we're reconnected
  if (!_mqtt->connected()) {
    LOGN("[MQTT] Retry connect...");
    // Attempt to connect
    if (_mqtt->connect(getClientId().c_str(), getUser().c_str(),
                       getPass().c_str())) {
      String msg = "[MQTT] Reconnected to ";
      msg += _server;
      msg += " as ";
      msg += getClientId();
      mqttFileLog(msg);
      sendOnline();
      _mqtt->subscribe("test");
      _mqtt->subscribe(getCmdTopic().c_str());
      mqttFileLog("Reconnected");
    } else {
      LOG("[MQTT] Reconnect failed, rc=");
      LOGN(_mqtt->state());
    }
  } else {
    // LOGN("[MQTT] Connection is stable");
    // mqttPing();
  }
}

void MqttManager::begin(CMD_HANDLER_FUNC handler) {
  _handler = handler;
  connect();
}

void MqttManager::loop() {
  _mqtt->loop();
}

void MqttManager::handleMessage(const char* _topic,
                                const uint8_t* _payload,
                                const unsigned int _length) {
  yield();
  string topic(_topic);
  string message(_payload, _payload + std::min(_length, COMMAND_MAX_LENGTH));
  // replace newline for log print
  message = extstring::replace_all(message, "\n", " ");
  extstring::trim(message);
  char logBuf[message.size() + topic.size() + 24];
  snprintf(logBuf, COMMAND_MAX_LENGTH + 24, "[MQTT][%s] %s (%d)", topic.c_str(),
           message.c_str(), _length);
  mqttFileLog(logBuf);
  if (!isCommand(topic)) {
    LOGN(F("[MQTT] Not a command"));
    sendLog("What?");
    return;
  }
  if (!checkCommand(message)) {
    LOGN(F("[MQTT] Command must start with /"));
    sendLog("What?");
    return;
  }
  if (_handler != nullptr) {
    vector<string> args = extstring::split_any(message);
    for (auto arg : args) {
      extstring::trim(arg);
    }
    _handler(args);
  }
}

void MqttManager::sendOnline() {
  // online message retain
  bool ret = _mqtt->publish(getStatusTopic().c_str(), "Online", true);
  if (!ret) {
    LOGN("[MQTT] mqtt online sent failed.");
  } else {
    LOGN("[MQTT] mqtt online sent successfully.");
  }
}