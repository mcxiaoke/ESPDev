#ifndef __ESP__COMPAT__
#define __ESP__COMPAT__

//#define DEBUG_MODE
#define USING_MQTT
#define EANBLE_LOGGING

#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
// #include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecureBearSSL.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
// #include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif

#endif