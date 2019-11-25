#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
// hardware I2C
// for esp32, GPIO21(SDA), GPIO22(SCL)
// esp32 good pins:
// 2,4,5,13-19,21-7,32,33
// for esp8266 nodemcu D1-4(SDA) D2-5(SCL)
// esp8266 good pins:
// d5-14,d6-12,d7-13,d8-15,(d3-0,d4-2)

// https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
class Display {
 public:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2;
  Display();
  Display(const Display& dp) = delete;
  Display& operator=(const Display& dp) = delete;
  ~Display();
  void begin();
  void clear();
  void setText(const String& a = "",
               const String& b = "",
               const String& c = "",
               const String& d = "");

 private:
};

#endif