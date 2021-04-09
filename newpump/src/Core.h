#ifndef __MCX_SRC_CORE_HEADER__
#define __MCX_SRC_CORE_HEADER__

#include <AFileServer.h>
#include <AWebServer.h>
#include <Arduino.h>
#include <MQTTManager.h>
#include <RelayUnit.h>
#include <build.h>
#include <rest.h>
#include <tools.h>

#include <memory>

#ifdef DEBUG
#define RUN_INTERVAL_DEFAULT 10 * 60 * 1000UL
#define RUN_DURATION_DEFAULT 18 * 1000UL
#else
#define RUN_INTERVAL_DEFAULT 24 * 3600 * 1000UL
#define RUN_DURATION_DEFAULT 60 * 1000UL
#endif

constexpr int led = LED_BUILTIN;

constexpr const char REBOOT_RESPONSE[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting...\n";

extern RelayUnit pump;
extern RestApi api;
extern MQTTManager mqttClient;
extern AWebServer webServer;
extern bool _should_reboot_hello;
void setupApp();
void setupApi();

#endif /* __MCX_SRC_CORE_HEADER__ */
