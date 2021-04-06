#include <MainModule.h>

static unsigned long startMillis;

AWebServer webServer(80);
MQTTManager mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

bool _required_setup_wifi() {
  Serial.println("_required_setup_wifi");
  AWiFi.onWiFiReady([]() { Serial.println("WiFi Ready"); });
  AWiFi.onWiFiLost([]() { Serial.println("WiFi Lost"); });
  AWiFi.setCredentials(WIFI_SSID, WIFI_PASS);
  return AWiFi.begin();
}

bool _required_setup_date() {
  Serial.println("_required_setup_date");
  if (!WiFi.isConnected()) {
    return false;
  }
  LOGN("[Setup] setupDate using ntp server1");
  DateTime.setTimeZone("CST-8");
  DateTime.setServer("cn.ntp.org.cn");
  DateTime.begin(10 * 1000L);

  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server2");
    DateTime.setServer("time.pool.aliyun.com");
    DateTime.begin(10 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN("[Setup] setupDate using ntp server3");
    DateTime.setServer("ntp.ntsc.ac.cn");
    DateTime.begin(10 * 1000L);
  }
  if (!DateTime.isTimeValid()) {
    LOGN(F("[Setup] Time sync failed, will reboot"));
  } else {
    LOGF("[Setup] Time sync using %lus.\n", (millis() - startMillis) / 1000);
    startMillis = millis();
  }
  return DateTime.isTimeValid();
}

void _required_before_setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(1000);
  Serial.println("=== SETUP:BEGIN ===");
  checkFileSystem();
  ALogger.init();
  LOGN("======@@@ Booting Begin @@@======");
}

void _required_after_setup() {
  Serial.println("_required_after_setup");
  LOGF("[Setup] Host: %s (%s)\n", compat::getHostName(),
       WiFi.localIP().toString());
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGF("[Setup] Booting using time: %ds\n", millis() / 1000);
  LOGN("======@@@ Booting Finished @@@======");
  Serial.println("=== SETUP:END ===");
}

void setup() {
  _required_before_setup();
  showESP("beforeWiFi");
  beforeWiFi();
  bool ret = _required_setup_wifi();
  if (!ret) {
    LOGN("[ERROR] WiFi Connect failed, will reboot.");
    compat::restart();
    return;
  }
  showESP("beforeDate");
  ret = _required_setup_date();
  if (!ret) {
    LOGN("[ERROR] Date Sync failed, will reboot.");
    compat::restart();
    return;
  }
  ALogger.begin();
  showESP("beforeServer");
  beforeServer();
  webServer.begin();
  showESP("beforeMQTT");
  mqtt.begin();
  setupLast();
  showESP("beforeLast");
  _required_after_setup();
}

void loop() {
  loopFirst();
  Timer.loop();
  AWiFi.loop();
  ALogger.loop();
  webServer.loop();
  mqtt.loop();
  loopLast();
}
