#include <AWiFiManager.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPUpdateServer.h>
#include <private.h>
#include <utils.h>

ESPUpdateServer otaUpdate;
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  fsCheck();
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println("Common Lib Example");
  AWiFiManager.begin(WIFI_SSID, WIFI_PASS);
  FileSerial.setup();
  UDPSerial.setup();
  otaUpdate.setup(&server);
  server.on("/serial", [](AsyncWebServerRequest* request) {
    request->send(FileFS, "/serial.log", "text/plain");
  });
  server.begin();
  MDNS.addService("http", "tcp", 80);
  LOGF("[Setup] %s (%s)\n", getHostName(), WiFi.localIP().toString());
  LOGN("[Setup] Everything is OK!");
}

void loop() {
  otaUpdate.loop();
  UDPSerial.run();
#if defined(ESP8266)
  MDNS.update();
#endif
}