#include "ServerIO.h"

using std::string;

void showRequest(AsyncWebServerRequest *request, bool showHeaders) {
  LOGNF("[IP]: %s", request->client()->remoteIP().toString());
  LOGNF("[REQ]: %s %s", request->methodToString(), request->url());

  if (showHeaders) {
    // List all collected headers
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      AsyncWebHeader *h = request->getHeader(i);
      Serial.printf("HEADER [%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
  }

  // List all parameters
  int params = request->params();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter *p = request->getParam(i);
    if (p->isFile()) {  // p->isPost() is also true
      LOGNF("FILE [%s]: %s, size: %u", p->name(), p->value(), p->size());
    } else if (p->isPost()) {
      LOGNF("POST [%s]: %s", p->name(), p->value());
    } else {
      LOGNF("GET [%s]: %s", p->name(), p->value());
    }
  }
}

void handleWebIO(AsyncWebServerRequest *r) {
  // showRequest(r);
  String path = r->url();
  LOGN("[GPIO] path:", path);
  auto ps = ext::string::split(string(r->url().c_str()), "/");
  // handle invalid path
  if (ps.size() != 2) {
    r->send(HTTP_CODE_BAD_REQUEST, MIME_TEXT_PLAIN, GPIO_HELP);
    return;
  }
  int statusCode = HTTP_CODE_BAD_REQUEST;
  int resCode = GPIO_ERROR_UNKNOWN;
  // handle invalid pin
  if (!ext::string::is_digits(ps[1])) {
    // error path, no pin
    statusCode = HTTP_CODE_BAD_REQUEST;
    resCode = GPIO_ERROR_PATH;
    r->send(statusCode, MIME_TEXT_PLAIN, String(resCode));
    LOGN("[GPIO] invalid pin");
    return;
  }
  int pin = atoi(ps[1].c_str());
  auto pAction = r->getParam("action");
  auto pValue = r->getParam("value");
  auto value = pValue ? pValue->value() : "";
  auto action = pAction ? pAction->value() : "";
  if (action && action.length() >= 4) {
    if (action == "mode") {
      if (value == "in") {
        // set pin to mode in
        pinMode(pin, INPUT);
        statusCode = HTTP_CODE_OK;
        resCode = INPUT;
      } else if (value == "out") {
        // set pin to mode out
        pinMode(pin, OUTPUT);
        statusCode = HTTP_CODE_OK;
        resCode = OUTPUT;
      } else {
        // error value
        statusCode = HTTP_CODE_BAD_REQUEST;
        resCode = GPIO_ERROR_VALUE;
      }
    } else if (action == "write") {
      if (value && !value.isEmpty() &&
          ext::string::is_digits(string(value.c_str()))) {
        // or using sscanf(s.c_str(), "%d", &i)
        // or strtol or atoi
        pinMode(pin, OUTPUT);
        int v = value.toInt();
        // digital write to pin
        if (v == LOW) {
          digitalWrite(pin, LOW);
          statusCode = HTTP_CODE_OK;
          resCode = LOW;
        } else if (v == HIGH) {
          digitalWrite(pin, HIGH);
          statusCode = HTTP_CODE_OK;
          resCode = HIGH;
        } else {
          // error value
          statusCode = HTTP_CODE_BAD_REQUEST;
          resCode = GPIO_ERROR_VALUE;
        }
      } else {
        // error value
        statusCode = HTTP_CODE_BAD_REQUEST;
        resCode = GPIO_ERROR_VALUE;
      }
    } else if (action == "awrite") {
      if (value && !value.isEmpty() &&
          ext::string::is_digits(string(value.c_str()))) {
        // pinMode(pin, OUTPUT);
        int v = value.toInt();
        // digital write to pin
        analogWrite(pin, v);
        statusCode = HTTP_CODE_OK;
        resCode = v;
      } else {
        // error value
        statusCode = HTTP_CODE_BAD_REQUEST;
        resCode = GPIO_ERROR_VALUE;
      }
    } else if (action == "read") {
      // digital read
      statusCode = HTTP_CODE_OK;
      resCode = digitalRead(pin);
    } else if (action == "aread") {
      // analog read
      // https://github.com/espressif/arduino-esp32/issues/683
      // ADC1 whit 8 channels attached to GPIOs 32-39
      statusCode = HTTP_CODE_OK;
      resCode = analogRead(pin);
    } else {
      // error action
      statusCode = HTTP_CODE_BAD_REQUEST;
      resCode = GPIO_ERROR_ACTION;
    }
  } else {
    // error action
    statusCode = HTTP_CODE_BAD_REQUEST;
    resCode = GPIO_ERROR_ACTION;
  }
  LOGNF("[GPIO] uri:%s, code:%d, return:%d", path, statusCode, resCode);
  r->send(statusCode, MIME_TEXT_PLAIN, String(resCode));
}
