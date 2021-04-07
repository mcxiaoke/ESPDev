
#include <Core.h>
#include <MainModule.h>

void onWiFiReady() {}

void onWiFiLost() {}

void beforeWiFi() {
  DLOG();
  pinMode(led, OUTPUT);
}
void beforeServer() {
  DLOG();
  setupServer();
}

void setupLast() {
  DLOG();
  setupCommands();
  setupTimers();
  setupPump();
}

void loopFirst() {}

void loopLast() { pump.run(); }