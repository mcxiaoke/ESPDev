#include <ALogger.h>

bool ALoggerClass::begin() {
  udp.setup();
  return true;
}

void ALoggerClass::init() { file.setup(); }

void ALoggerClass::loop() { udp.loop(); }

void ALoggerClass::setFlags(uint8_t _flags) { flags = _flags; }

void ALoggerClass::flush() {
  if (flags & LOGGER_FLAG_SERIAL) {
    Serial.flush();
  }
  if (flags & LOGGER_FLAG_FILE) {
    file.flush();
  }
  if (flags & LOGGER_FLAG_UDP) {
    udp.flush();
  }
}

size_t ALoggerClass::log(const char* msg) {
  size_t n = 0;
  if (flags & LOGGER_FLAG_SERIAL) {
    n = Serial.print(msg);
  }
  if (flags & LOGGER_FLAG_FILE) {
    n = file.print(msg);
  }
  if (flags & LOGGER_FLAG_UDP) {
    n = udp.print(msg);
  }
  return n;
}
size_t ALoggerClass::logn(const char* s) {
  size_t n = 0;
  if (flags & LOGGER_FLAG_SERIAL) {
    n = Serial.println(s);
  }
  if (flags & LOGGER_FLAG_FILE) {
    n = file.println(s);
  }
  if (flags & LOGGER_FLAG_UDP) {
    n = udp.println(s);
  }
  return n;
}

size_t ALoggerClass::log(const String& s) { return this->log(s.c_str()); }
size_t ALoggerClass::logn(const String& s) { return this->logn(s.c_str()); }

size_t ALoggerClass::ulog(const char* msg) {
  Serial.println(msg);
  return udp.print(msg);
}

size_t ALoggerClass::ulog(const String& s) {
  Serial.println(s);
  return udp.print(s);
}

ALoggerClass ALogger;