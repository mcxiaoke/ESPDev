#ifndef __MCX_LIBCOMMON_AMODULEMANAGER_HEADER__
#define __MCX_LIBCOMMON_AMODULEMANAGER_HEADER__

#include <AModule.h>

#include <vector>

using namespace std;

class AModuleManagerClass : AModuleInterface {
 private:
  vector<AModuleInterface*> modules;

 public:
  AModuleManagerClass(/* args */);
  ~AModuleManagerClass();
  const char* getModuleName() { return "AModuleManager"; }
  bool begin() { return true; };
  void loop(){};
  void add(AModuleInterface* module) { modules.push_back(module); }
  void clear() { modules.clear(); }
};

extern AModuleManagerClass AModuleManager;

#endif /* __MCX_LIBCOMMON_AMODULEMANAGER_HEADER__ */
