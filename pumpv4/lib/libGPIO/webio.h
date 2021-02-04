#ifndef ESP_DEV_WEB_GPIO_H
#define ESP_DEV_WEB_GPIO_H

#include <compat.h>
#include <utils.h>
// espserver lib include begin
#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
// espserver lib include end

constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";
constexpr const char WEBIO_ACTIONS[] PROGMEM =
    "=== Web GPIO API Instructions === \n\n\
/gpio/10/out - pinMode() set output mode\n\
/gpio/10/in - pinMode() set input mode\n\
/gpio/10/write/0 - digitalWrite() write digital value\n\
/gpio/10/read - digitalRead() read digital value\n\
/gpio/10/awrite/99 - analogWrite() write analog value\n\
/gpio/10/aread - analogRead() read analog value\n\
";

void showRequest(AsyncWebServerRequest *request, bool showHeaders = false);
void handleWebIO(AsyncWebServerRequest *r);

#endif