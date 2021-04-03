#include "UDPSerial.h"

#define UDP_SERIAL_PORT 10000

void UDPSerialClass::before() {
  if (!conneted) {
    return;
  }
  IPAddress ip = WiFi.localIP();
  ip[3] = 255;
  udp.beginPacket(ip, UDP_SERIAL_PORT);
  // udp.write('$');
}

void UDPSerialClass::end() {
  if (!conneted) {
    return;
  }
  udp.endPacket();
}

void UDPSerialClass::setup() {
  if (compat::isWiFiConnected()) {
    uint8_t ret = udp.begin(UDP_SERIAL_PORT);
    conneted = (ret == 1);
  } else {
    conneted = false;
  }
}

int UDPSerialClass::available() { return true; }

int UDPSerialClass::peek() { return 1; }
int UDPSerialClass::read() { return 1; }
void UDPSerialClass::flush() {}
size_t UDPSerialClass::write(uint8_t c) { return 0; }
size_t UDPSerialClass::print(char c) { return 0; }
size_t UDPSerialClass::print(int c) { return 0; }
size_t UDPSerialClass::print(unsigned int c) { return 0; }
size_t UDPSerialClass::print(long c) { return 0; }
size_t UDPSerialClass::print(unsigned long c) { return 0; }

size_t UDPSerialClass::print(const char* msg) {
  if (!conneted) {
    return 0;
  }
  if (!msg) {
    return 0;
  }
  if (strlen(msg) < 3) {
    return 0;
  }
  this->before();
#if defined(ESP8266)
  size_t nw = udp.write(msg);
#else
  size_t nw = udp.write((const uint8_t*)msg, strlen_P(msg));
#endif
  this->end();
  return nw;
}
size_t UDPSerialClass::println(const char* s) { return this->print(s); }

size_t UDPSerialClass::print(const String& s) { return this->print(s.c_str()); }
size_t UDPSerialClass::println(const String& s) {
  return this->print(s.c_str());
}

UDPSerialClass UDPSerial;