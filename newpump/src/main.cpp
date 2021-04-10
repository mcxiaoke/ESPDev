
#include <Core.h>
#include <MainModule.h>

void onWiFiReady() {}

void onWiFiLost() {}

void beforeWiFi() { DLOG(); }
void beforeServer() {
  DLOG();
  setupApi();
}

void setupLast() {
  DLOG();
  setupApp();
}

void loopFirst() {}

void loopLast() { pump.run(); }