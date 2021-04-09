#include <App.h>
#include <MainModule.h>

void beforeWiFi() {}
void beforeServer() { setupApp(); }
void setupLast() {}
void loopFirst() {}
void loopLast() {}
void onWiFiReady() {}
void onWiFiLost() {}