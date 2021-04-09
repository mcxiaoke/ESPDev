#ifndef ESP_DEV_MQTT_H
#define ESP_DEV_MQTT_H

#include <ACommand.h>
#include <ADebug.h>
#include <AModule.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <compat.h>

#define CUSTOM_MQTT_KEEPALIVE 30

using std::string;

using MQTT_CALLBACK_FUNC = std::function<void(char*, uint8_t*, unsigned int)>;

class MQTTManager : public AModuleInterface {
 public:
  // https://stackoverflow.com/questions/11522399
  constexpr static unsigned int COMMAND_MAX_LENGTH = 128;
  constexpr static const char* TOPIC_DEVICE_CHECK = "device/check";
  constexpr static const char* TOPIC_DEVICE_ONLINE = "device/online";
  constexpr static const char* CMD_DEVICE_CHECK = "check";
  MQTTManager(const char* server, const int port, const char* username,
              const char* password);
  MQTTManager(const MQTTManager& m) = delete;
  MQTTManager& operator=(const MQTTManager& dp) = delete;
  const char* getModuleName() { return "MQTTManager"; }
  bool isConnected();
  String getUser();
  String getPass();
  String getClientId();
  void sendStatus(const String& text);
  void sendLog(const String& text);
  void sendSerial(const String& text);
  void connect();
  void check();
  void setHandler(CMD_HANDLER_FUNC);
  bool begin();
  void loop();
  void mute(bool silent);

 private:
  const char* _server;
  const int _port;
  const char* _username;
  const char* _password;
  bool _silentMode;
  int _lastState;
  unsigned long _lastOnlineMs;
  unsigned long _lastOfflineMs;
  unsigned long _lastCheckMs;
  unsigned long _lastLoopCallMs;
  WiFiClient _client;
  PubSubClient* _mqtt;
  CMD_HANDLER_FUNC _handler;
  void checkStateChange();
  void handleMessage(const char* topic, const uint8_t* payload,
                     const unsigned int length);
  boolean sendMessage(const char* topic, const char* payload,
                      boolean retained = false);
  void sendOnline();
  void initSubscribe();
  string getStatusTopic();
  string getLogTopic();
  string getCmdTopic();
  string getSerialTxTopic();
  string getSerialRxTopic();
};

#endif