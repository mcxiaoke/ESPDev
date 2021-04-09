
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
  setupApi();
}

void setupLast() {
  DLOG();
  setupApp();
}

void loopFirst() {
  if (_should_reboot_hello) {
    compat::restart();
    delay(2000);
  }
}

void loopLast() { pump.run(); }