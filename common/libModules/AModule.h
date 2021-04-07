#ifndef __MCX_LIBCOMMON_AMODULE_HEADER__
#define __MCX_LIBCOMMON_AMODULE_HEADER__

class AModuleInterface {
  virtual const char* getModuleName() = 0;
  virtual bool begin() = 0;
  virtual void loop() = 0;
};

class ANetworkModuleInterface : public AModuleInterface {
  virtual void onWiFiReady() = 0;
  virtual void onWiFiLost() = 0;
};

#endif /* __MCX_LIBCOMMON_AMODULE_HEADER__ */
