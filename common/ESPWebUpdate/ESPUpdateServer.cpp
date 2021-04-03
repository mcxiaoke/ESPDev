#include "ESPUpdateServer.h"

#include "StreamString.h"

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

ESPUpdateServer::ESPUpdateServer(bool serial_debug, const String& username,
                                 const String& password)
    : _server(nullptr),
      _username(username),
      _password(password),
      _authenticated(false) {}

void ESPUpdateServer::setup(AsyncWebServer* server, const String& path,
                            const String& username, const String& password) {
  _server = server;
  _username = username;
  _password = password;

  _server->on(path.c_str(), HTTP_GET, [&](AsyncWebServerRequest* request) {
    handleUpdatePage(request);
  });

  // handler for the /update form POST (once file upload finishes)
  _server->on(
      path.c_str(), HTTP_POST,
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
}

void ESPUpdateServer::_setUpdaterError() {
  Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str;
}

void ESPUpdateServer::handleUpdatePage(AsyncWebServerRequest* request) {
  // handler for the /update form page
  if (_username != emptyString && _password != emptyString &&
      !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();
  if (false && FileFS.exists("/update.html")) {
    request->send(FileFS, "/update.html");
  } else {
    request->send_P(200, PSTR("text/html"), serverIndex);
  }
}

void ESPUpdateServer::handleUploadEnd(AsyncWebServerRequest* request) {
  if (!_authenticated) return request->requestAuthentication();
  PLOGN("[OTA] Update End!");
  delay(500);
  if (!Update.hasError()) {
    AsyncWebServerResponse* response;
    if (request->hasHeader("X-Source")) {
      response = request->beginResponse(200, "application/json", jsonResponse);
    } else {
      response = request->beginResponse(200, "text/html", successResponse);
      response->addHeader("Refresh", "15");
      response->addHeader("Location", "/");
    }
    response->addHeader("Connection", "close");
    request->send(response);
    delay(500);
    request->client()->stop();
    request->client()->close();
    PLOGN("[OTA] update process finished");
    fileLog("[OTA] successed at " + dateTimeString());
    delay(500);
    writeFile(FIRMWARE_UPDATE_FILE, dateTimeString(), false);
    _shouldRestart = true;
  } else {
    request->send(200, "text/html",
                  String(F("Update error: ")) + _updaterError + "\n");
    delay(500);
    fileLog("[OTA] failed at " + dateTimeString());
    _shouldRestart = false;
  }
}

void ESPUpdateServer::handleUploadProgress(size_t progress, size_t total) {
  static size_t nextChunk = 50 * 1024;
  if (progress > nextChunk) {
    // progressMs = millis();
    nextChunk += 50 * 1024;
    PLOGF("[OTA] Upload progress: %d%% (%d)\n", (progress * 100) / total,
          progress);
  }
}

void ESPUpdateServer::handleUpload(AsyncWebServerRequest* request,
                                   const String& filename, size_t index,
                                   uint8_t* data, size_t len, bool final) {
  _authenticated =
      (_username == emptyString || _password == emptyString ||
       request->authenticate(_username.c_str(), _password.c_str()));
  if (!_authenticated) {
    PLOGF("[OTA] Unauthenticated Update\n");
    return;
  }
  size_t binSize = request->contentLength();
  if (!index) {
    PLOGN("[OTA] update process init stage");
    // fileLog("[OTA] updated from " + ESP.getSketchMD5());
    _updaterError = String();
    int cmd = (filename.indexOf("spiffs") > -1) ? CMD_FS : CMD_FLASH;
    PLOGF("[OTA] Update Firmware: %s\n", filename.c_str());
    PLOGF("[OTA] Update Type: %s\n", cmd == CMD_FS ? "FS" : "FLASH");

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
      handleUploadProgress(index + len, binSize);
#endif
    }
  }
  if (final) {
    PLOGN("[OTA] update process final stage");
    if (!Update.end(true)) {
      _setUpdaterError();
      PLOGN("[OTA] update failed!");
    } else {
      PLOGN("[OTA] update success!");
    }
  }
}

void ESPUpdateServer::loop() {
  if (_shouldRestart) {
    LOGN("[OTA] update completed, now reboot.");
    _shouldRestart = false;
    delay(200);
    compat::restart();
  }
}