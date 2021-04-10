#include "App.h"

static WiFiClient client;
LanDevice pump{"DF975D", "ESP Plant Watering Device", "192.168.1.116", 80};
LanDevice n1box{"N1BOX", "N1 Armbian Home Server", "192.168.1.114", 80};
// LanDevice n54l{"N54L", "Windows 7 NAS Server", "192.168.1.110", 80};

void sendMessage(String title, String content = "") {
  String wxUrl = WX_REPORT_URL;
  wxUrl += "?title=";
  wxUrl += urlencode(title);
  if (content && content.length() > 0) {
    wxUrl += "&desp=";
    wxUrl += urlencode(content);
  }
  wifiHttpGet(wxUrl, client);
}

void sendDeviceOnline() {
  sendMessage(
      "LAN Monitor " + compat::getHostName() + " Online",
      "IP: " + WiFi.localIP().toString() + F(" (Lan Device Online Monitor)"));
}

bool checkOpen(const LanDevice& device) {
  client.setTimeout(5000);
  client.connect(device.host, device.port);
  bool online = client.connected();
  client.stop();
  return online;
}

void checkDevice(LanDevice& device, bool forceMsg = false) {
  bool online = checkOpen(device);
  if (online != device.online || forceMsg) {
    device.online = online;
    sendMessage(device.msgTitle(), device.msgDesp());
    LOGN("[Monitor] " + device.toString());
  }
}

void checkPorts(bool forceMsg) {
  Timer.setTimeout(1000L, [forceMsg]() { checkDevice(pump, forceMsg); });
  Timer.setTimeout(3000L, [forceMsg]() { checkDevice(n1box, forceMsg); });
}

void handleCheck(AsyncWebServerRequest* request) {
  request->send(200, MIME_TEXT_PLAIN, "OK");
  LOGN("[Monitor] Check ports from HTTP");
  checkPorts(true);
}

void setupApp() {
  auto server = webServer.getServer();
  ULOGN("[Monitor] Add web handler for /check");
  server->on("/check", handleCheck);
  ULOGN("[Monitor] Add check ports timers");
  // check ports open every 5 minutes
  Timer.setInterval(
      2 * 60 * 1000L, []() { checkPorts(false); }, "checkPorts");
  // send online message every 12 hours
  Timer.setInterval(12 * 60 * 60 * 1000L, sendDeviceOnline,
                    "send_device_online");
  checkPorts(true);
  sendDeviceOnline();
}