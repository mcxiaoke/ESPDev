#ifndef _RELAY_UNIT_
#define _RELAY_UNIT_

#include <memory>
#include <string>
#include <ArduinoTimer.h>
#include "compat.h"
#include "utils.h"

struct RelayConfig;
struct RelayStatus;

struct RelayConfig {
  const char* name;
  uint8_t pin;
  unsigned long interval;
  unsigned long duration;

  std::string toString() const;
};

struct RelayStatus {
  bool enabled = true;
  unsigned long setupAt;
  unsigned long timerResetAt;
  unsigned long lastStart;
  unsigned long lastStop;
  unsigned long lastElapsed;
  unsigned long totalElapsed;
  std::string toString() const;
};
// enum value can conflict with #define variable
enum class RelayEvent { Started, Stopped, Enabled, Disabled, ConfigChanged };

class RelayUnit {
 public:
  using CALLBACK_FUNC = std::function<void(const RelayEvent, int reason)>;
  RelayUnit();
  explicit RelayUnit(const RelayConfig& cfg);
  void begin(const RelayConfig& cfg);
  void run();
  bool start();
  bool stop();
  bool isOn();  // running
  bool isEnabled();
  void setEnabled(bool ena);
  void setCallback(CALLBACK_FUNC cb);
  uint8_t pin();
  uint8_t pinValue();
  int updateConfig(const RelayConfig& config);
  std::shared_ptr<RelayConfig> getConfig();
  std::shared_ptr<RelayStatus> getStatus();

 private:
  std::shared_ptr<RelayConfig> pConfig;
  std::shared_ptr<RelayStatus> pStatus;
  ArduinoTimer timer{"replay"};
  int runTimerId = -1;
  int checkTimerId = -1;
  int stopTimerId = -1;
  CALLBACK_FUNC callback;
  void check();
  void resetTimer();
};

#endif