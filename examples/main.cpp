#include <ADebug.h>
#include <AFileUtils.h>
#include <AWebServer.h>
#include <AWiFiManager.h>
#include <Arduino.h>
#include <MQTTManager.h>
#include <private.h>

unsigned long startMillis;
AWebServer server(80);
MQTTManager mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

void setupWiFi() {
  AWiFi.onWiFiReady([]() { Serial.println("WiFi Ready"); });
  AWiFi.onWiFiLost([]() { Serial.println("WiFi Lost"); });
  AWiFi.setCredentials(WIFI_SSID, WIFI_PASS);
  AWiFi.begin();
}

bool setupDate() {
  Serial.println("setupDate");
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

void beforeSetup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(1000);
  Serial.println("=== SETUP:BEGIN ===");
  checkFileSystem();
  ALogger.init();
  LOGN("======@@@ Booting Begin @@@======");
}

void afterSetup() {
  LOGF("[Setup] WiFi: %s (%s)\n", compat::getHostName(),
       WiFi.localIP().toString());
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGF("[Setup] Booting using time: %ds\n", millis() / 1000);
  LOGN("======@@@ Booting Finished @@@======");
  Serial.println("=== SETUP:END ===");
}

void setup() {
  beforeSetup();
  setupWiFi();
  setupDate();
  ALogger.begin();
  server.begin();
  mqtt.begin();
  afterSetup();
}

void loop() {
  AWiFi.loop();
  ALogger.loop();
  server.loop();
  mqtt.loop();
}