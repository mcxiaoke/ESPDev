#ifndef ESP_SERIAL_UTILS_HEADER
#define ESP_SERIAL_UTILS_HEADER

#include <Arduino.h>

inline String buildMessage(const String s) {
  // char buf[8];
  // sprintf(buf, "%08lu", millis());
  // String message = "";
  // message += "[";
  // message += buf;
  // message += "] ";
  // message += s;
  return s;
}

inline String buildMessage(const char* s) { return buildMessage(String(s)); }

#endif