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
    : _serial_output(serial_debug),
      _server(nullptr),
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

  if (_serial_output) {
    Serial.setDebugOutput(true);
  }
}

void ESPUpdateServer::_setUpdaterError() {
  if (_serial_output) Update.printError(Serial);
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
  Serial.printf("[OTA] Update End!\n");
  delay(100);
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
    delay(200);
    request->client()->stop();
    request->client()->close();
    if (_serial_output) {
      Serial.println("[OTA] update process finished");
      Serial.flush();
    }

    fileLog("[OTA] successed at " + dateTimeString());
    writeFile(FIRMWARE_UPDATE_FILE, dateTimeString(), false);
    delay(200);
    _shouldRestart = true;
  } else {
    request->send(200, "text/html",
                  String(F("Update error: ")) + _updaterError + "\n");
    delay(200);
    fileLog("[OTA] failed at " + dateTimeString());
    _shouldRestart = false;
  }
}

void ESPUpdateServer::handleUploadProgress(size_t progress, size_t total) {
  static size_t nextChunk = 50 * 1024;
  if (progress > nextChunk) {
    // progressMs = millis();
    nextChunk += 50 * 1024;
    if (_serial_output) {
      Serial.printf("[OTA] Upload progress: %d%% (%d)\n",
                    (progress * 100) / total, progress);
      // Serial.flush();
    }
  }
}

void ESPUpdateServer::handleUpload(AsyncWebServerRequest* request,
                                   const String& filename, size_t index,
                                   uint8_t* data, size_t len, bool final) {
  _authenticated =
      (_username == emptyString || _password == emptyString ||
       request->authenticate(_username.c_str(), _password.c_str()));
  if (!_authenticated) {
    if (_serial_output) Serial.printf("[OTA] Unauthenticated Update\n");
    return;
  }
  size_t binSize = request->contentLength();
  if (!index) {
    Serial.println("[OTA] update process init stage");
    fileLog("[OTA] updated from " + ESP.getSketchMD5());
    _updaterError = String();
    int cmd = (filename.indexOf("spiffs") > -1) ? CMD_FS : CMD_FLASH;
    if (_serial_output) {
      Serial.printf("[OTA] Update Firmware: %s\n", filename.c_str());
      Serial.printf("[OTA] Update Type: %s\n", cmd == CMD_FS ? "FS" : "FLASH");
    }
#ifdef ESP8266
    Update.runAsync(true);
#endif

    if (cmd == CMD_FS) {
#if defined(ESP8266)
      close_all_fs();
#endif
      LOGN("====fsSize=", compat::flashSize());
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
    Serial.println("[OTA] update process final stage");
    if (!Update.end(true)) {
      _setUpdaterError();
      fileLog("[OTA] update failed!");
    } else {
      fileLog("[OTA] update success!");
    }
  }
}

void ESPUpdateServer::loop() {
  if (_shouldRestart) {
    PLOGN("[OTA] update completed, now reboot.");
    _shouldRestart = false;
    delay(100);
    ESP.restart();
  }
}