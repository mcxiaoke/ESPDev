#ifndef ARDUINO_A_LOGGER_H
#define ARDUINO_A_LOGGER_H

#include <AFileLogger.h>
#include <AUDPLogger.h>

#define LOGGER_FLAG_DISABLE (1 << 0)
#define LOGGER_FLAG_SERIAL (1 << 1)
#define LOGGER_FLAG_FILE (1 << 2)
#define LOGGER_FLAG_UDP (1 << 3)
#define LOGGER_FLAG_MQTT (1 << 4)

class ALoggerClass {
 private:
#ifdef DEBUG
  uint8_t flags = LOGGER_FLAG_SERIAL | LOGGER_FLAG_FILE | LOGGER_FLAG_UDP;
#else
  uint8_t flags = LOGGER_FLAG_SERIAL | LOGGER_FLAG_FILE | LOGGER_FLAG_UDP;
#endif

 protected:
  AFileLogger file;
  AUDPLogger udp;

 public:
  ALoggerClass(){};
  ~ALoggerClass(){};
  const char* getModuleName() { return "ALogger"; }
  void init();
  bool begin();
  void loop();
  void setFlags(uint8_t _flag);
  void flush();
  void clear() { file.clear(); }
  size_t log(const char* s);
  size_t logn(const char* s);
  size_t log(const String& s);
  size_t logn(const String& s);
  size_t ulog(const char* s);
  size_t ulog(const String& s);
};

extern ALoggerClass ALogger;

#endif