#ifndef ESP_DEV_MQTT_H
#define ESP_DEV_MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ACommand.h>
#include <config.h>
#include <utils.h>

using std::string;

using MQTT_CALLBACK_FUNC = std::function<void(char*, uint8_t*, unsigned int)>;

class MqttManager {
 public:
  // https://stackoverflow.com/questions/11522399
  constexpr static unsigned int COMMAND_MAX_LENGTH = 128;
  constexpr static const char* TOPIC_DEVICE_CHECK = "device/check";
  constexpr static const char* TOPIC_DEVICE_ONLINE = "device/online";
  constexpr static const char* CMD_DEVICE_CHECK = "check";
  MqttManager(const char* server,
              const int port,
              const char* username,
              const char* password);
  MqttManager(const MqttManager& m) = delete;
  MqttManager& operator=(const MqttManager& dp) = delete;
  bool isConnected();
  String getUser();
  String getPass();
  String getClientId();
  void sendStatus(const String& text);
  void sendLog(const String& text);
  void sendSerial(const String& text);
  void connect();
  void check();
  void begin(CMD_HANDLER_FUNC);
  void loop();
  void mute(bool silent);

 private:
  const char* _server;
  const int _port;
  const char* _username;
  const char* _password;
  bool _silentMode;
  unsigned long _lastOnlineMs;
  unsigned long _lastOfflineMs;
  WiFiClient _client;
  PubSubClient* _mqtt;
  CMD_HANDLER_FUNC _handler;
  void handleStateChange(int state);
  void handleMessage(const char* topic,
                     const uint8_t* payload,
                     const unsigned int length);
  boolean sendMessage(const char* topic,
                      const char* payload,
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