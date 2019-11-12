#include "display.h"

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // working, full buffer
// U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0); // working, page buffer

Display::Display()
    : u8g2(new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0)), buffer({}) {}

void Display::begin() {
  u8g2->begin();
  // enable UTF8 support for the Arduino print()
  u8g2->enableUTF8Print();
  u8g2->setFont(u8g2_font_wqy12_t_gb2312a);
  u8g2->setFontDirection(0);
}

void Display::clear() {
  u8g2->clearBuffer();
  u8g2->sendBuffer();
}

void Display::setLine1(const String& s, bool flush) {
  buffer[0] = s;
  u8g2->setCursor(2, 20);
  u8g2->print(s);
  if (flush)
    u8g2->sendBuffer();
}

void Display::setLine2(const String& s, bool flush) {
  buffer[1] = s;
  u8g2->setCursor(2, 33);
  u8g2->print(s);
  if (flush)
    u8g2->sendBuffer();
}

void Display::setLine3(const String& s, bool flush) {
  buffer[2] = s;
  u8g2->setCursor(2, 46);
  u8g2->print(s);
  if (flush)
    u8g2->sendBuffer();
}

void Display::setLine4(const String& s, bool flush) {
  buffer[3] = s;
  u8g2->setCursor(2, 59);
  u8g2->print(s);
  if (flush)
    u8g2->sendBuffer();
}

void Display::setText(const String& a,
                      const String& b,
                      const String& c,
                      const String& d) {
  u8g2->clearBuffer();
  setLine1(a, false);
  setLine2(b, false);
  setLine3(c, false);
  setLine4(d, false);
  u8g2->sendBuffer();
}