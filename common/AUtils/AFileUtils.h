#ifndef ESP_DEV_UTILS_H
#define ESP_DEV_UTILS_H

#include <DateTime.h>
#include <MD5Builder.h>
#include <compat.h>

#include <ext/string.hpp>
#include <ext/utils.hpp>

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

void checkFileSystem();
std::vector<std::tuple<String, size_t>> listFiles(const char* dest = "/");
size_t writeLine(const String& path, const String& line);
size_t writeFile(const String& path, const String& content, bool append = true);
String readFile(const String& path);

#endif