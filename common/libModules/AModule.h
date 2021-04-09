#ifndef __MCX_LIBCOMMON_AMODULE_HEADER__
#define __MCX_LIBCOMMON_AMODULE_HEADER__

class AModuleInterface {
 private:
  bool _safeMode;
  bool _shouldRestart;

 public:
  const char* getModuleName() { return "Module"; };
  bool begin() { return true; };
  void loop(){};
  void setSafeMode(bool value) { this->_safeMode = value; }
  bool isSafeMode() { return this->_safeMode; }
  void setShouldRestart(bool value) { this->_shouldRestart = value; }
  bool shouldRestart() { return this->_shouldRestart; }
};

class ANetworkModuleInterface : public AModuleInterface {
  virtual void onWiFiReady() = 0;
  virtual void onWiFiLost() = 0;
};

#endif /* __MCX_LIBCOMMON_AMODULE_HEADER__ */
