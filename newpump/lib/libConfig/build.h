#ifndef ESP_DEV_BUILD_H
#define ESP_DEV_BUILD_H

#include "config.h"

#define STR1(x) #x
#define STR(x) STR1(x)

//#define USING_BLYNK

#if !defined(DISABLE_MQTT)
#define USING_MQTT
#endif

#if !defined(DISABLE_LOG) || defined(DEBUG)
#define EANBLE_LOGGING
#endif

#ifdef BUILD_VERSION
#define APP_VERSION STR(BUILD_VERSION)
#else
#define APP_VERSION "-"
#endif

// fix for travis ci build begin
#ifndef WIFI_SSID
#define WIFI_SSID "x"
#define WIFI_PASS "x"
#endif
#ifndef MQTT_SERVER
#define MQTT_SERVER "x.x.x"
#define MQTT_PORT 10000
#define MQTT_USER "x"
#define MQTT_PASS "x"
#endif
#ifndef BLYNK_AUTH
#define BLYNK_AUTH "x"
#define BLYNK_HOST "x.x.x"
#define BLYNK_PORT 10000
#endif
#ifndef WX_REPORT_URL
#define WX_REPORT_URL "x.x.x"
#endif
// fix for travis ci build end

#endif