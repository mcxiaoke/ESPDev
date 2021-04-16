#include <MainModule.h>

static const char* NTP_SERVERS[] = {"ntp.ntsc.ac.cn", "cn.ntp.org.cn",
                                    "time.pool.aliyun.com", "time.apple.com",
                                    "edu.ntp.org.cn"};

static unsigned long runningMs;
AWebServer webServer(80);
MQTTManager mqttClient(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS);

String _main_get_app_name() { return compat::getHostName(); }

bool _main_setup_wifi() {
  DLOG();
  AWiFi.onWiFiReady(onWiFiReady);
  AWiFi.onWiFiLost(onWiFiLost);
  AWiFi.setCredentials(WIFI_SSID, WIFI_PASS);
  return AWiFi.begin();
}

bool _main_setup_date() {
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

void _main_live_check() {
  LOGF("[Core] Everything is OK! <%s> (%s)\n", SafeMode.isEnabled() ? "S" : "N",
       humanTimeMs(millis()));
}

void _main_send_online() {
  if (!WiFi.isConnected()) {
    return;
  }
  ULOGF("%s Online <%s> (%s)", _main_get_app_name(),
        SafeMode.isEnabled() ? "S" : "N", humanTimeMs(millis()));
}

void _main_setup_safe_mode() {
  if (SafeMode.getLastSketchMD5() != ESP.getSketchMD5()) {
    // firmware changed, clear safe mode
    // Serial.println("[Core] Sketch changed, disable Safe Mode");
    SafeMode.setEnable(false);
  }
  SafeMode.setup();
#ifdef ESP8266
  String reason = ESP.getResetReason();
  String info = ESP.getResetInfo();
  if ((reason && reason.indexOf("Exception") != -1) ||
      (info && info.indexOf("Exception") != -1)) {
    SafeMode.setEnable(true);
  }
#endif
  // Serial.println("[Core] Reset Reason: " + reason);
  // Serial.println("[Core] Reset Info: " + info);
  // Serial.printf("[Core] Safe Mode: %s\n",
  //               SafeMode.isEnabled() ? "true" : "false");
}

void _main_before_setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(1000);
  DLOG();
  Serial.println("=== SETUP:BEGIN ===");
  checkFileSystem();
  ALogger.init();
  _main_setup_safe_mode();
  LOGN("======@@@ Booting Begin @@@======");
  if (SafeMode.isEnabled()) {
    LOGN("***** Boot on Safe Mode. *****");
  }
}

void _main_setup_modules() {
  auto e = SafeMode.isEnabled();
  ALogger.setSafeMode(e);
  AWiFi.setSafeMode(e);
  AUpdateServer.setSafeMode(e);
  AFileServer.setSafeMode(e);
  webServer.setSafeMode(e);
  mqttClient.setSafeMode(e);
}

void _main_after_setup() {
  DLOG();
  runningMs = millis();
  LOGF("[Setup] Build: %s\n", __TIMESTAMP__);
  LOGF("[Setup] Host: %s (%s)\n", compat::getHostName(),
       WiFi.localIP().toString());
  LOGN("[Setup] Sketch: " + ESP.getSketchMD5());
  LOGN("[Setup] Date: " + DateTime.toISOString());
  LOGF("[Setup] Booting using time: %ds\n", runningMs / 1000);
#if defined(DEBUG) || defined(DEBUG_ESP_PORT)
  LOGN("[Setup] Debug Mode");
#else
  LOGN("[Setup] Release Mode");
#endif
  if (SafeMode.isEnabled()) {
    LOGN("***** Boot on Safe Mode. *****");
  }
  LOGN("======@@@ Booting Finished @@@======");
  Serial.println("=== SETUP:END ===");
  SafeMode.saveLastSketchMD5();
}

void setup() {
  _main_before_setup();
  _main_setup_modules();
  if (!SafeMode.isEnabled()) {
    beforeWiFi();
  }
  bool ret = _main_setup_wifi();
  if (!ret) {
    LOGN("[ERROR] WiFi Connect failed, will reboot.");
    compat::restart();
    return;
  }
  ALogger.begin();
  ret = _main_setup_date();
  if (!ret) {
    LOGN("[ERROR] Date Sync failed, will reboot.");
    compat::restart();
    return;
  }
  if (!SafeMode.isEnabled()) {
    beforeServer();
  }
  webServer.begin();
  if (!SafeMode.isEnabled()) {
    mqttClient.begin();
    Timer.setInterval(4 * 60 * 60 * 1000L, _main_live_check,
                      "_main_live_check");
    Timer.setInterval(60 * 1000L, _main_send_online, "_main_send_online");
    setupLast();
  }
  _main_after_setup();
  // wrong use delay may cause crash
  // https://github.com/khoih-prog/Blynk_WM/issues/24
}

void loop() {
  webServer.loop();
  AWiFi.loop();
  ALogger.loop();
  if (!SafeMode.isEnabled()) {
    // only on normal mode
    loopFirst();
    Timer.loop();
    mqttClient.loop();
    loopLast();
  }
}
