#include "ESPUpdateServer.h"
#include "StreamString.h"
#include "compat.h"

static const char serverIndex[] PROGMEM =
    R"(<html><body><form method='POST' action='' enctype='multipart/form-data'>
                  <input type='file' name='update'>
                  <input type='submit' value='Update'>
               </form>
         </body></html>)";
static const char successResponse[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"20;URL=/\">Update Success! "
    "Rebooting...\n";

ESPUpdateServer::ESPUpdateServer(bool serial_debug) {
  _serial_output = serial_debug;
  _server = NULL;
  _username = emptyString;
  _password = emptyString;
  _authenticated = false;
}

void ESPUpdateServer::setup(AsyncWebServer* server,
                            const String& path,
                            const String& username,
                            const String& password) {
  _server = server;
  _username = username;
  _password = password;

  _server->on(path.c_str(), HTTP_GET,
              [&](AsyncWebServerRequest* request) { handleUpdate(request); });

  // handler for the /update form POST (once file upload finishes)
  _server->on(path.c_str(), HTTP_POST,
              [&](AsyncWebServerRequest* request) { handleUploadEnd(request); },
              [&](AsyncWebServerRequest* request, String filename, size_t index,
                  uint8_t* data, size_t len, bool final) {
                handleUpload(request, filename, index, data, len, final);
              });

  Update.onProgress([&](size_t progress, size_t total) {
    handleUploadProgress(progress, total);
  });
}

void ESPUpdateServer::_setUpdaterError() {
  if (_serial_output)
    Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str.c_str();
}

void ESPUpdateServer::handleUpdate(AsyncWebServerRequest* request) {
  // handler for the /update form page
  if (_username != emptyString && _password != emptyString &&
      !request->authenticate(_username.c_str(), _password.c_str()))
    return request->requestAuthentication();
  request->send_P(200, PSTR("text/html"), serverIndex);
}

void ESPUpdateServer::handleUploadEnd(AsyncWebServerRequest* request) {
  if (!_authenticated)
    return request->requestAuthentication();
  bool shouldReboot = !Update.hasError();
  if (_serial_output) {
    Serial.printf("\n[OTA] Rebooting... %d\n", shouldReboot);
    Serial.flush();
  }
  if (shouldReboot) {
    ESP.restart();
  }
}

void ESPUpdateServer::handleUploadProgress(size_t progress, size_t total) {
  if (_serial_output) {
    Serial.printf("[OTA] Progress: %d%%\n", (progress * 100) / total);
  }
}

void ESPUpdateServer::handleUpload(AsyncWebServerRequest* request,
                                   const String& filename,
                                   size_t index,
                                   uint8_t* data,
                                   size_t len,
                                   bool final) {
  _authenticated =
      (_username == emptyString || _password == emptyString ||
       request->authenticate(_username.c_str(), _password.c_str()));
  if (!_authenticated) {
    if (_serial_output)
      Serial.printf("[OTA] Unauthenticated Update\n");
    return;
  }
  if (!index) {
    _updaterError = String();
    if (_serial_output)
      Serial.setDebugOutput(true);
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    if (_serial_output) {
      Serial.printf("[OTA] Update Start: %s\n", filename.c_str());
      Serial.printf("[OTA] Update Type: %s\n",
                    cmd == U_FLASH ? "U_FLASH" : "U_SPIFFS");
    }
#if defined(ESP8266)
    Update.runAsync(true);
#endif
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace, cmd)) {
      // start with max available size
      _setUpdaterError();
    }
  }

  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      _setUpdaterError();
    }
  }
  if (final) {
    request->send(200, "text/html", successResponse);
    if (!Update.end(true)) {
      _setUpdaterError();
    } else {
      if (_serial_output) {
        Serial.printf("\n[OTA] Update Success\n");
        Serial.flush();
      }
    }
  }
}
