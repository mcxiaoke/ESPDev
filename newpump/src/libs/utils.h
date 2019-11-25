#ifndef __UTILS_H__
#define __UTILS_H__

#include <MD5Builder.h>
#include "compat.h"
#include "log.h"
#include "ntp.h"
#include "tools.h"

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