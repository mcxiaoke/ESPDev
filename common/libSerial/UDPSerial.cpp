#include "UDPSerial.h"

#define UDP_SERIAL_PORT 10000

void UDPSerialClass::before() {
  IPAddress ip = WiFi.localIP();
  ip[3] = 255;
  udp.beginPacket(ip, UDP_SERIAL_PORT);
  udp.write('$');
}

void UDPSerialClass::end() { udp.endPacket(); }

void UDPSerialClass::setup() {
  uint8_t ret = udp.begin(UDP_SERIAL_PORT);
  conneted = ret == 1;
}

int UDPSerialClass::available() { return true; }

int UDPSerialClass::peek() { return 1; }
int UDPSerialClass::read() { return 1; }
void UDPSerialClass::flush() {}
size_t UDPSerialClass::write(uint8_t c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  udp.write(c);
  this->end();
  return sizeof(uint8_t);
  ;
}
size_t UDPSerialClass::print(char c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  udp.write(c);
  this->end();
  return sizeof(char);
  ;
}
size_t UDPSerialClass::print(int c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  udp.write(c);
  this->end();
  return sizeof(int);
  ;
}
size_t UDPSerialClass::print(unsigned int c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  udp.write(c);
  this->end();
  return sizeof(unsigned int);
  ;
}
size_t UDPSerialClass::print(long c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  udp.write(c);
  this->end();
  return sizeof(long);
}
size_t UDPSerialClass::print(unsigned long c) {
  if (!conneted) {
    return 0;
  }
  this->before();
  size_t n = udp.write(c);
  this->end();
  return n;
}

size_t UDPSerialClass::print(const char* s) {
  if (!conneted) {
    return 0;
  }
  this->before();

  size_t n = strlen(s) + 1;
  uint8_t* s2 = new uint8_t[n];
  memcpy(s2, s, n);
  size_t nw = udp.write(s2, n);
  delete[] s2;
  this->end();
  return nw;
}
size_t UDPSerialClass::println(const char* s) { return this->print(s); }

size_t UDPSerialClass::print(const String& s) {
  return this->print(buildMessage(s).c_str());
}
size_t UDPSerialClass::println(const String& s) {
  return this->println(buildMessage(s).c_str());
}

UDPSerialClass UDPSerial;