#ifndef __MCX_SRC_APP_HEADER__
#define __MCX_SRC_APP_HEADER__

#include <MainModule.h>
#include <net.h>

void setupApp();

struct LanDevice {
  const char* name;
  const char* info;
  const char* host;
  const uint16_t port;
  bool online;
  unsigned long lastOnlineMs;

  String toString() const {
    return String(name) + "/" + String(host) + ":" + String(port) + " " +
           (online ? "Online" : "Offline");
  }

  String msgTitle() {
    return "Device " + String(name) + (online ? " Online" : " Offline");
  }

  String msgDesp() { return "IP: " + String(host) + " (" + String(info) + ")"; }
};

#endif /* __MCX_SRC_APP_HEADER__ */
