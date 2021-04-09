#ifndef __MCX_LIBDEBUG_SAFEMODE_HEADER__
#define __MCX_LIBDEBUG_SAFEMODE_HEADER__

#include <Arduino.h>
#include <FS.h>

static const char* SAFE_MODE_FILE = "/boot_safe_mode";

class SafeModeClass {
 private:
  bool _safeMode;
  void updateModeFile() {
    if (_safeMode) {
      File f = SPIFFS.open(SAFE_MODE_FILE, "w");
      f.write("Safe Mode Enabled");
      f.close();
    } else {
      SPIFFS.remove(SAFE_MODE_FILE);
    }
  }

 public:
  SafeModeClass(){};
  ~SafeModeClass(){};
  void setup() { _safeMode = SPIFFS.exists(SAFE_MODE_FILE); }
  bool setEnable(bool newMode) {
    if (_safeMode != newMode) {
      _safeMode = newMode;
      updateModeFile();
      return true;
    }
    return false;
  }
  bool isEnabled() { return _safeMode; }
};

extern SafeModeClass SafeMode;

#endif /* __MCX_LIBDEBUG_SAFEMODE_HEADER__ */
