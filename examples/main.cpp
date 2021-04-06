#include <Arduino.h>
#include <MainModule.h>

unsigned long lastMs;

void beforeWiFi() { Serial.println("beforeWiFi()"); }
void beforeServer() {
  Serial.println("beforeServer()");
  auto server = webServer.getServer();
  server.on("/hello", [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello, World!");
  });
}

void setupLast() { Serial.println("setupLast()"); }

void loopFirst() {
  if (millis() - lastMs > 15 * 1000L) {
    lastMs = millis();
    Serial.println("loopFirst()");
  }
}

void loopLast() {}