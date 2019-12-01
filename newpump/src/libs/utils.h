#ifndef __UTILS_H__
#define __UTILS_H__

#include <MD5Builder.h>
#include <DateTime.h>
#include <ext/format.hpp>
#include <ext/string.hpp>
#include <ext/utility.hpp>
#include <ALogger.h>
#include "compat.h"
#include "tools.h"

// https://forum.arduino.cc/index.php/topic,46643.0.html
// Serial << "hello,World" << "End\n";
template <typename Arg>
inline Print& operator<<(Print& obj, Arg arg) {
  obj.print(arg);
  return obj;
}

std::vector<std::tuple<String, size_t>> listFiles();
void fsCheck();
String getDevice();
String getMD5(uint8_t* data, uint16_t len);
String getMD5(const char* data);
String getMD5(const String& data);
void showESP();
String logFileName(const String& suffix = "");
size_t fileLog(const String& text,
               const String& path = logFileName(),
               bool appendDate = true);
size_t _writeLog(const String& text, const String& path);
String readLog(const String& path);

bool hasValidTime();
time_t setTimestamp();
time_t getTimestamp();
time_t getBootTime();
String dateString();
String dateTimeString();
String timeString();

#endif