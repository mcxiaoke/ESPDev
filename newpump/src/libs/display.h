#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
class Display {
 public:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2;
  Display();
  void begin();
  void clear();
  void setText(const String& a = "",
               const String& b = "",
               const String& c = "",
               const String& d = "");

 private:
};

#endif