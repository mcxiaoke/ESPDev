#include <Arduino.h>
#include <MainModule.h>

// ==============================
// main.app must include <MainModule.h>
// main.cpp should impl this function
// in setup() before wifi connected
// extern void beforeWiFi();
// in setup() before web server begin
// extern void beforeServer();
// in setup() before setup() end
// extern void setupLast();
// in loop() head
// extern void loopFirst();
// in loop() last
// extern void loopLast();
// on wifi ready (connected/reconnected)
// extern void onWiFiReady();
// on wifi lost
// extern void onWiFiLost();
// ==============================

void beforeServer() {
  DLOG();
  auto server = webServer.getServer();
  server->on("/hello", [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello, World!");
  });
}

void setupLast() { DLOG(); }

void onWiFiReady() { DLOG(); }
void onWiFiLost() { DLOG(); }