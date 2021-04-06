#ifndef ESP_SERIAL_UTILS_HEADER
#define ESP_SERIAL_UTILS_HEADER

#include <Arduino.h>
#include <DateTime.h>

inline String buildMessage(const String s) {
  // char buf[9];
  // sprintf(buf, "%08lu", millis() / 1000);
  String message = "";
  message += "[";
  message += DateTime.toString();
  message += "] ";
  message += s;
  return message;
}

inline String buildMessage(const char* s) { return buildMessage(String(s)); }

#endif