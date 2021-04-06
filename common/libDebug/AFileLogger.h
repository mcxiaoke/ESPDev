#ifndef ESP_FILE_SERIAL_HEADER
#define ESP_FILE_SERIAL_HEADER

#include <compat.h>

#include "SerialUtils.h"

#define FILE_SERIAL_NAME "/serial.log"

class AFileLogger : public Stream {
 public:
  AFileLogger();
  ~AFileLogger();

  void setup();
  void end();
  int available() override;
  int peek() override;
  int read() override;
  void flush() override;
  size_t write(uint8_t c) override;
  size_t print(char c);
  size_t print(int c);
  size_t print(unsigned int c);
  size_t print(long c);
  size_t print(unsigned long c);
  size_t print(const char* s);
  size_t println(const char* s);
  size_t print(const String& s);
  size_t println(const String& s);

 protected:
  const char* path = FILE_SERIAL_NAME;
  File fp;

  File ensureFile();
};

#endif