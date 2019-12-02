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

#endif