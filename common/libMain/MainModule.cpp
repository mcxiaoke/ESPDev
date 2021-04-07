#include <MainModule.h>

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
  LOGN("[Setup] setupDate using ntp server1");
  DateTime.setTimeZone("CST-8");
  DateTime.setServer("cn.ntp.org.cn");
  DateTime.begin(5 * 1000L);

  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server2");
    DateTime.setServer("time.pool.aliyun.com");
    DateTime.begin(5 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server3");
    DateTime.setServer("ntp.ntsc.ac.cn");
    DateTime.begin(5 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server4");
    DateTime.setServer("time.apple.com");
    DateTime.begin(5 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server5");
    DateTime.setServer("edu.ntp.org.cn");
    DateTime.begin(5 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN(F("[Setup] Time sync failed, will reboot"));
  } else {
    LOGF("[Setup] Time sync using %lus.\n", (millis() - runningMs) / 1000);
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
  LOGF("[Setup] Build At %s\n", __TIMESTAMP__);
  LOGF("[Setup] Host: %s (%s)\n", compat::getHostName(),
       WiFi.localIP().toString());
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGF("[Setup] Booting using time: %ds\n", runningMs / 1000);
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
