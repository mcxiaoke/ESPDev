#ifndef __MCX_LIBDEBUG_ADEVDEBUG_HEADER__
#define __MCX_LIBDEBUG_ADEVDEBUG_HEADER__

#include <Arduino.h>

#define MAKE_STR1(x) #x
#define MAKE_STR(x) MAKE_STR1(x)

// better use https://github.com/bblanchon/ArduinoTrace
#ifdef DEBUG_ENABLE_ALL
#define LINENO "[" __FILE__ "#" MAKE_STR(__LINE__) "]"
#define _DLOG_PRINT(fmt, ...)                                      \
  do {                                                             \
    Serial.printf(PSTR("[DEBUG]" LINENO fmt "\n"), ##__VA_ARGS__); \
  } while (0)
// #define DLOG() DLOG("%s", __PRETTY_FUNCTION__)
#define DLOG(fmt, ...) _DLOG_PRINT("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define DLOG(x...) \
  do {             \
    (void)0;       \
  } while (0)
#endif

#endif /* __MCX_LIBDEBUG_ADEVDEBUG_HEADER__ */
