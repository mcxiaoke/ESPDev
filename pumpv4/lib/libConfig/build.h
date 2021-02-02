#ifndef ESP_DEV_BUILD_H
#define ESP_DEV_BUILD_H

#include "config.h"

#define STR1(x) #x
#define STR(x) STR1(x)

#if !defined(DISABLE_LOG)
#define EANBLE_LOGGING
#endif

// fix for travis ci build begin
#ifndef WIFI_SSID
#define WIFI_SSID "x"
#define WIFI_PASS "x"
#endif

#endif