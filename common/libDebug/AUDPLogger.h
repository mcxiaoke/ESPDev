#ifndef ESP_UDP_SERIAL_HEADER
#define ESP_UDP_SERIAL_HEADER

#include <compat.h>

#include <queue>

#include "SerialUtils.h"

struct StringMessage {
  const String content;
  const unsigned long createdAt;
  bool fromQ;
};

class AUDPLogger : public Stream {
 public:
  void setup();
  void before();
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
  void loop();

 protected:
  std::queue<StringMessage> pms;
  WiFiUDP udp;
  bool conneted;
  void processQueue();
  void handleCmd(const char* cmd);
};

#endif