#ifndef ESP_DEV_UTILS_H
#define ESP_DEV_UTILS_H

#include <ALogger.h>
#include <DateTime.h>
#include <MD5Builder.h>
#include <compat.h>

#include <ext/format.hpp>
#include <ext/string.hpp>
#include <ext/utility.hpp>

#include "tools.h"

// #include <umm_malloc/umm_malloc.h>

constexpr const size_t block_size = 8;

// inline size_t getTotalAvailableMemory() {
//   umm_info(0, 0);
//   return ummHeapInfo.freeBlocks * block_size;
// }

// inline size_t getLargestAvailableBlock() {
//   umm_info(0, 0);
//   return ummHeapInfo.maxFreeContiguousBlocks * block_size;
// }

// https://forum.arduino.cc/index.php/topic,46643.0.html
// Serial << "hello,World" << "End\n";
template <typename Arg>
inline Print& operator<<(Print& obj, Arg arg) {
  obj.print(arg);
  return obj;
}

std::vector<std::tuple<String, size_t>> listFiles(const char* dest = "/");
std::vector<std::tuple<String, size_t>> listLogs();
void fsCheck();
String getUDID();
String getHostName();
String getMD5(uint8_t* data, uint16_t len);
String getMD5(const char* data);
String getMD5(const String& data);
void showESP(const char* extra = "");
String logFileName(const String& suffix = "");
size_t fileLog(const String& text, const String& path = logFileName(),
               bool appendDate = true);
bool trimLogFile(String fileName = logFileName());
size_t writeLine(const String& path, const String& line);
size_t writeFile(const String& path, const String& content, bool append = true);
String readFile(const String& path);

bool hasValidTime();
time_t setTimestamp();
time_t getTimestamp();
time_t getBootTime();
String dateString();
String dateTimeString();
String timeString();

#endif