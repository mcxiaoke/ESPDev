#ifndef ESP_DEV_BUILD_H
#define ESP_DEV_BUILD_H

#include "config.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#if !defined(DISABLE_LOG)
#define EANBLE_LOGGING
#endif

// #ifdef BUILD_REV
// #define APP_REVISION STR(BUILD_REV)
// #else
// #define APP_REVISION "0000"
// #endif

#endif