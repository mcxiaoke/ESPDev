#include <MainModule.h>

static const char* NTP_SERVERS[] = {"ntp.ntsc.ac.cn", "cn.ntp.org.cn",
                                    "time.pool.aliyun.com", "time.apple.com",
                                    "edu.ntp.org.cn"};

static unsigned long runningMs;
AWebServer webServer(80);
MQTTManager mqttClient(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

bool _setup_wifi() {
  DLOG();
  AWiFi.onWiFiReady(onWiFiReady);
  AWiFi.onWiFiLost(onWiFiLost);
  AWiFi.setCredentials(WIFI_SSID, WIFI_PASS);
  return AWiFi.begin();
}

bool _setup_date() {
  DLOG();
  if (!WiFi.isConnected()) {
    return false;
  }
  runningMs = millis();
  DateTime.setTimeZone("CST-8");
  size_t count = sizeof(NTP_SERVERS) / sizeof(NTP_SERVERS[0]);
  for (size_t i = 0; i < count; i++) {
    auto ntpServer = NTP_SERVERS[i];
    LOGF("[Date] NTP Sync with %s\n", ntpServer);
    DateTime.setServer(ntpServer);
    DateTime.begin(5 * 1000L);
    if (DateTime.isTimeValid()) {
      break;
    }
  }
  if (!DateTime.isTimeValid()) {
    LOGN(F("[Date] NTP Sync failed, will reboot"));
  } else {
    LOGF("[Date] NTP Sync using time: %lums.\n", millis() - runningMs);
    runningMs = millis();
  }
  return DateTime.isTimeValid();
}

void _before_setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(1000);
  DLOG();
  Serial.println("=== SETUP:BEGIN ===");
  checkFileSystem();
  ALogger.init();
  LOGN("======@@@ Booting Begin @@@======");
}

void _after_setup() {
  DLOG();
  runningMs = millis();
  LOGF("[Setup] Build: %s\n", __TIMESTAMP__);
  LOGF("[Setup] Host: %s (%s)\n", compat::getHostName(),
       WiFi.localIP().toString());
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGF("[Setup] Booting using time: %ds\n", runningMs / 1000);
#if defined(DEBUG) || defined(DDEBUG_ESP_PORT)
  LOGN("[Setup] Debug Mode");
#else
  LOGN("[Setup] Release Mode");
#endif
  LOGN("======@@@ Booting Finished @@@======");
  Serial.println("=== SETUP:END ===");
}

void setup() {
  _before_setup();
  beforeWiFi();
  bool ret = _setup_wifi();
  if (!ret) {
    LOGN("[ERROR] WiFi Connect failed, will reboot.");
    compat::restart();
    return;
  }
  ALogger.begin();
  ret = _setup_date();
  if (!ret) {
    LOGN("[ERROR] Date Sync failed, will reboot.");
    compat::restart();
    return;
  }
  beforeServer();
  webServer.begin();
  mqttClient.begin();
  setupLast();
  _after_setup();
}

void loop() {
  loopFirst();
  AWiFi.loop();
  webServer.loop();
  Timer.loop();
  ALogger.loop();
  mqttClient.loop();
  loopLast();
}
