#ifndef ESP_UDP_SERIAL_HEADER
#define ESP_UDP_SERIAL_HEADER

#include <compat.h>
#include "SerialUtils.h"

class UDPSerialClass : public Stream {
 public:
  UDPSerialClass(){};
  ~UDPSerialClass(){};

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

 protected:
  WiFiUDP udp;
  bool conneted;
};

extern UDPSerialClass UDPSerial;  // define in cpp

#endif