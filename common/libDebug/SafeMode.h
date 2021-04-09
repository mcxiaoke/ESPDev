#ifndef __MCX_LIBDEBUG_SAFEMODE_HEADER__
#define __MCX_LIBDEBUG_SAFEMODE_HEADER__

#include <Arduino.h>
#include <FS.h>
// disable non-core modules
static const char* SAFE_MODE_FILE = "/boot_safe_mode";
// sketch md5 compare and check
static const char* SKETCH_MD5_FILE = "/last_sketch_md5.txt";

class SafeModeClass {
 private:
  bool _safeMode;
  void _updateSafeMode() {
    if (_safeMode) {
      File f = SPIFFS.open(SAFE_MODE_FILE, "w");
      f.write("S");
      f.close();
    } else {
      SPIFFS.remove(SAFE_MODE_FILE);
    }
  };

 public:
  SafeModeClass(){};
  ~SafeModeClass(){};
  void setup() { _safeMode = SPIFFS.exists(SAFE_MODE_FILE); }
  bool setEnable(bool newMode) {
    if (_safeMode != newMode) {
      Serial.printf("[SafeMode] setEnable(%s)\n", newMode ? "true" : "false");
      _safeMode = newMode;
      _updateSafeMode();
      return true;
    }
    return false;
  }
  bool isEnabled() { return _safeMode; }
  String getLastSketchMD5() {
    File f = SPIFFS.open(SKETCH_MD5_FILE, "r");
    if (f) {
      String s = f.readString();
      f.close();
      return s;
    } else {
      return ESP.getSketchMD5();
    }
  }
  void saveLastSketchMD5() {
    File f = SPIFFS.open(SKETCH_MD5_FILE, "w");
    if (f) {
      f.print(ESP.getSketchMD5());
      f.close();
    }
  }
};

extern SafeModeClass SafeMode;

#endif /* __MCX_LIBDEBUG_SAFEMODE_HEADER__ */
