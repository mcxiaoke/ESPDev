#ifndef __MCX_LIBMAIN_MAINMODULE_HEADER__
#define __MCX_LIBMAIN_MAINMODULE_HEADER__

#include <ADebug.h>
#include <AFileUtils.h>
#include <AModule.h>
#include <ATimer.h>
#include <AWebServer.h>
#include <AWiFiManager.h>
#include <Arduino.h>
#include <MQTTManager.h>
#include <SafeMode.h>
#include <private.h>
#include <tools.h>

extern AWebServer webServer;
extern MQTTManager mqttClient;

extern void beforeWiFi();
extern void beforeServer();
extern void setupLast();
extern void loopFirst();
extern void loopLast();
extern void onWiFiReady();
extern void onWiFiLost();

void setup();
void loop();

/***********************
 *
 * MAIN.CPP TEMPLATE
 *
#include <App.h>
#include <MainModule.h>

String getAppName();
void beforeWiFi() {}
void beforeServer() {}
void setupLast() {}
void loopFirst() {}
void loopLast() {}
void onWiFiReady() {}
void onWiFiLost() {}

*
*
***********************/

#endif /* __MCX_LIBMAIN_MAINMODULE_HEADER__ */
