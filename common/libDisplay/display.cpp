#include "display.h"

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // working, full buffer
// U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0); // working, page buffer

Display::Display() : u8g2(new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0)) {}

Display::~Display() {
  u8g2->clear();
  u8g2 = nullptr;
}

void Display::begin() {
  u8g2->begin();
  // enable UTF8 support for the Arduino print()
  //   u8g2->enableUTF8Print();
  u8g2->setFont(u8g2_font_profont12_tf);
  u8g2->setFontDirection(0);
}

void Display::clear() {
  u8g2->clearBuffer();
  u8g2->sendBuffer();
}

void Display::setText(const String& a,
                      const String& b,
                      const String& c,
                      const String& d) {
  u8g2->clearBuffer();
  u8g2->setCursor(2, 20);
  u8g2->print(a);
  u8g2->setCursor(2, 33);
  u8g2->print(b);
  u8g2->setCursor(2, 46);
  u8g2->print(c);
  u8g2->setCursor(2, 59);
  u8g2->print(d);
  u8g2->sendBuffer();
}