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
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! "
    "Rebooting...";

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

  _server->on(path.c_str(), HTTP_GET,
              [&](AsyncWebServerRequest* request) { handleUpdate(request); });

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

void ESPUpdateServer::handleUpdate(AsyncWebServerRequest* request) {
  // handler for the /update form page
  if (_username != emptyString && _password != emptyString &&
      !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();
  request->send_P(200, PSTR("text/html"), serverIndex);
}

void ESPUpdateServer::handleUploadEnd(AsyncWebServerRequest* request) {
  if (!_authenticated) return request->requestAuthentication();
  if (_serial_output) Serial.printf("[OTA] Update End!\n");
  if (!Update.hasError()) {
    AsyncWebServerResponse* response =
        request->beginResponse(200, "text/html", successResponse);
    response->addHeader("Connection", "close");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (_serial_output) {
      Serial.printf("\n[OTA] Rebooting... \n");
      Serial.flush();
    }
    // delay(100);
    // request->client()->stop();
    // request->client()->close();
    _shouldRestart = true;
    // ESP.restart();
  } else {
    request->send(200, "text/html",
                  String(F("Update error: ")) + _updaterError + "\n");
  }
}

void ESPUpdateServer::handleUploadProgress(size_t progress, size_t total) {
  if (millis() - progressMs > 2000) {
    progressMs = millis();
    if (_serial_output) {
      Serial.printf("[OTA] Progress: %d%%\n", (progress * 100) / total);
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
    _updaterError = String();
    int cmd = (filename.indexOf("spiffs") > -1) ? CMD_FS : CMD_FLASH;
    if (_serial_output) {
      Serial.printf("[OTA] Update FileName: %s\n", filename.c_str());
      Serial.printf("[OTA] Update Type: %s\n", cmd == CMD_FS ? "FS" : "FLASH");
    }
#ifdef ESP8266
    Update.runAsync(true);
#endif

    if (cmd == CMD_FS) {
      // size_t fsSize = ((size_t)&_FS_end - (size_t)&_FS_start);
      // close_all_fs();
      if (!Update.begin(compat::fsSize(), CMD_FS)) {
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
    if (!Update.end(true)) {
      _setUpdaterError();
      if (_serial_output) Serial.printf("[OTA] update failed!\n");
    } else {
      if (_serial_output) Serial.printf("[OTA] update success!\n");
    }
  }
}

void ESPUpdateServer::loop() {
  if (_shouldRestart) {
    _shouldRestart = false;
    delay(100);
    ESP.restart();
  }
}