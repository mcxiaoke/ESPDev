#include "AUpdateServer.h"

#include <StreamString.h>

#if defined(ESP8266)
#define CMD_FS U_FS
#elif defined(ESP32)
#define CMD_FS U_SPIFFS
#endif
#define CMD_FLASH U_FLASH

static const char serverIndex[] PROGMEM =
    R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <div><h1>OTA Update</h1></div>
     <div></div>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Upload'>
     </form>
     </body>
     </html>)";
static const char successResponse[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Now "
    "Rebooting...";
static const char jsonResponse[] PROGMEM = "{\"code\":0,\"msg\":\"ok\"}";

AUpdateServerClass::AUpdateServerClass(bool serial_debug,
                                       const String& username,
                                       const String& password)
    : _server(nullptr), _username(username), _password(password) {}

void AUpdateServerClass::setup(std::shared_ptr<AsyncWebServer> server,
                               const String& path, const String& username,
                               const String& password) {
  _server = server;
  _path = path;
  _username = username;
  _password = password;
}

void AUpdateServerClass::_setUpdaterError() {
  Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str;
}

void AUpdateServerClass::handleUpdatePage(AsyncWebServerRequest* request) {
  // handler for the /update form page
  if (_username != emptyString && _password != emptyString &&
      !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();
  if (FileFS.exists("/update.html")) {
    request->send(FileFS, "/update.html");
  } else {
    request->send_P(200, PSTR("text/html"), serverIndex);
  }
}

void AUpdateServerClass::handleUploadEnd(AsyncWebServerRequest* request) {
  ULOGN("[OTA] Update End!");
  // delay(500);
  if (!Update.hasError()) {
    auto userAgent = request->getHeader("User-Agent");
    bool fromCurl =
        userAgent != nullptr && userAgent->value().indexOf("curl") != -1;
    AsyncWebServerResponse* response;
    if (fromCurl || request->hasHeader("X-Source")) {
      response =
          request->beginResponse_P(200, "application/json", jsonResponse);
    } else {
      response = request->beginResponse_P(200, "text/html", successResponse);
      response->addHeader("Refresh", "15");
      response->addHeader("Location", "/");
    }
    request->send(response);
    delay(1000);
    request->client()->stop();
    request->client()->close();
    SafeMode.setEnable(false);
    LOGN("[OTA] Update process done");
    delay(1000);
    setShouldRestart(true);
  } else {
    request->send(200, "text/html",
                  String(F("Update error: ")) + _updaterError + "\n");
    delay(1000);
    LOGN("[OTA] Update process abort");
    delay(1000);
    setShouldRestart(false);
  }
}

void AUpdateServerClass::handleUploadProgress(size_t progress, size_t total) {
  static size_t nextChunk = 40 * 1024;
  if (progress > nextChunk) {
    // progressMs = millis();
    nextChunk += 40 * 1024;
    ULOGF("[OTA] Upload progress: %d%% (%dk/%dk)", (progress * 100) / total,
          progress / 1000, total / 1000);
  }
}

void AUpdateServerClass::handleUpload(AsyncWebServerRequest* request,
                                      const String& filename, size_t index,
                                      uint8_t* data, size_t len, bool final) {
  if (!index) {
    ULOGN("[OTA] Update process INIT STAGE");
    LOGN("[OTA] Update Old: " + ESP.getSketchMD5());
    _updaterError = String();
    int cmd = (filename.indexOf("spiffs") > -1) ? CMD_FS : CMD_FLASH;
    LOGF("[OTA] Update File: %s Type: %s \n", filename.c_str(),
         cmd == CMD_FS ? "FileSystem" : "Firmware");

#ifdef ESP8266
    Update.runAsync(true);
#endif

    if (cmd == CMD_FS) {
#if defined(ESP8266)
      close_all_fs();
#endif
      if (!Update.begin(compat::flashSize(), CMD_FS)) {
        _setUpdaterError();
      }
    } else {
      uint32_t maxSketchSpace =
          (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace, CMD_FLASH)) {
        _setUpdaterError();
      }
    }
  }
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      _setUpdaterError();
    } else {
#ifdef ESP8266
      size_t binSize = request->contentLength();
      handleUploadProgress(index + len, binSize);
#endif
    }
  }
  if (final) {
    ULOGN("[OTA] Update process FINAL STAGE");
    if (!Update.end(true)) {
      _setUpdaterError();
      LOGN("[OTA] Update Failed!");
    } else {
      LOGN("[OTA] Update Success!");
    }
  }
}

bool AUpdateServerClass::begin() {
  LOGN("[OTA] Setup Update Server");
  _server->rewrite("/u", "/update");
  _server->on(_path.c_str(), HTTP_GET, [&](AsyncWebServerRequest* request) {
    handleUpdatePage(request);
  });

  // handler for the /update form POST (once file upload finishes)
  _server->on(
      _path.c_str(), HTTP_POST,
      [&](AsyncWebServerRequest* request) { handleUploadEnd(request); },
      [&](AsyncWebServerRequest* request, String filename, size_t index,
          uint8_t* data, size_t len, bool final) {
        handleUpload(request, filename, index, data, len, final);
      });

#ifdef ESP32
  Update.onProgress([&](size_t progress, size_t total) {
    handleUploadProgress(progress, total);
  });
#endif
  return true;
}

void AUpdateServerClass::loop() {
  if (shouldRestart()) {
    LOGN("[OTA] Update Finished, Reboot.");
    setShouldRestart(false);
    delay(1000);
    compat::restart();
  }
}

AUpdateServerClass AUpdateServer;