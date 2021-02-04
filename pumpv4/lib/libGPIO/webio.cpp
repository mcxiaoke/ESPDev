#include "webio.h"

using std::string;

void showRequest(AsyncWebServerRequest *request, bool showHeaders) {
  Serial.printf("[REQ]: %s %s\n", request->methodToString(),
                request->url().c_str());
  Serial.printf("[IP]: %s\n", request->client()->remoteIP().toString().c_str());

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
      Serial.printf("FILE [%s]: %s, size: %u\n", p->name().c_str(),
                    p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Serial.printf("POST [%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("GET [%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
}

void handleWebIO(AsyncWebServerRequest *r) {
  showRequest(r);
  String path = r->url();
  LOGN("[GPIO] path:", path);
  // gpio/2?action=mode&value=in
  // action=mode&value=out
  // action=read|aread
  // action=write|awrite&value=0|1|987
}

// deprecated function
void handleWebIOPath(AsyncWebServerRequest *r) {
  const string MODE_OUT("out");
  const string MODE_IN("in");
  const string D_WRITE("write");
  const string D_READ("read");
  const string A_WRITE("awrite");
  const string A_READ("aread");
  //   showRequest(r);
  String path = r->url();
  LOGN("[GPIO] path:", path);
  auto ps = ext::string::split(string(r->url().c_str()), "/");
  if (ps.size() == 1) {
    r->send(200, MIME_TEXT_PLAIN, WEBIO_ACTIONS);
    return;
  }
  if (ps.size() < 3 || ps.size() > 4) {
    r->send(400);
    return;
  }
  int pin = atoi(ps[1].c_str());
  int resCode = 200;
  String resText = "9";
  if (ps.size() == 3) {
    if (ps[2] == MODE_OUT) {
      pinMode(pin, OUTPUT);
      resText = String(OUTPUT);
    } else if (ps[2] == MODE_IN) {
      pinMode(pin, INPUT);
      resText = String(INPUT);
    } else if (ps[2] == D_READ) {
      resText = String(digitalRead(pin));
    } else if (ps[2] == A_READ) {
      // https://github.com/espressif/arduino-esp32/issues/683
      // ADC1 whit 8 channels attached to GPIOs 32-39
      resText = String(analogRead(pin));
    } else {
      resCode = 400;
      resText = "-1";
    }
  } else if (ps.size() == 4) {
    if (ps[2] == D_WRITE) {
      auto v = atoi(ps[3].c_str());
      digitalWrite(pin, v == 0 ? LOW : HIGH);
      resText = String(v);
    } else if (ps[2] == A_WRITE) {
      auto v = atoi(ps[3].c_str());
      analogWrite(pin, v);
      resText = String(v);
    } else {
      resCode = 400;
      resText = "-1";
    }
  }
  fileLog("[GPIO]" + path + "," + resCode);
  r->send(resCode, MIME_TEXT_PLAIN, resText);
}