#ifndef __MCX_LIBMAIN_MAINMODULE_HEADER__
#define __MCX_LIBMAIN_MAINMODULE_HEADER__

#include <ADebug.h>
#include <AFileUtils.h>
#include <ATimer.h>
#include <AWebServer.h>
#include <AWiFiManager.h>
#include <Arduino.h>
#include <MQTTManager.h>
#include <private.h>

extern AWebServer webServer;
extern MQTTManager mqtt;

extern void beforeWiFi();
extern void beforeServer();
extern void setupLast();
extern void loopFirst();
extern void loopLast();

void _setupWiFi();

bool _setupDate();

void setup();

void loop();

#endif /* __MCX_LIBMAIN_MAINMODULE_HEADER__ */
