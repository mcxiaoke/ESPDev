#ifndef ESP_DEV_SERVER_GPIO_H
#define ESP_DEV_SERVER_GPIO_H

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

#define GPIO_ERROR_VALUE 101
#define GPIO_ERROR_ACTION 102
#define GPIO_ERROR_PATH 103
#define GPIO_ERROR_UNKNOWN 127

constexpr const char MIME_TEXT_PLAIN[] PROGMEM = "text/plain";
constexpr const char MIME_TEXT_HTML[] PROGMEM = "text/html";
constexpr const char GPIO_HELP[] PROGMEM =
    "=== HTTP GPIO API Instructions === \n\n\
POST|GET /gpio/10?action=mode&value=in|out - pinMode()\n\
POST|GET /gpio/10?action=write&value=1 - digitalWrite()\n\
POST|GET /gpio/10?action=awrite&value=99 - analogWrite()\n\
GET /gpio/10?action=read - digitalRead()\n\
GET /gpio/10?action=aread - analogRead()\n\
";

struct IOResult {
    const char* uri;
    int statusCode;
    int errorCode;
};

void showRequest(AsyncWebServerRequest *request, bool showHeaders = false);
void handleWebIO(AsyncWebServerRequest *r);

#endif