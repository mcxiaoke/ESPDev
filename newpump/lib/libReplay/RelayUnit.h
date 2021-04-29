#ifndef ESP_DEV_RELAY_UNIT
#define ESP_DEV_RELAY_UNIT

#include <ADebug.h>
#include <ATimer.h>
#include <compat.h>

#include <ext/string.hpp>
#include <ext/utils.hpp>
#include <memory>
#include <string>

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
  unsigned long setupAt;
  unsigned long timerResetAt;
  unsigned long lastStart;
  unsigned long lastStop;
  unsigned long lastElapsed;
  unsigned long totalElapsed;
  void reset();
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
  bool isOn() const;  // running
  bool isTimerEnabled() const;
  void setTimerEnabled(bool ena);
  void reset();
  void setCallback(CALLBACK_FUNC cb);
  uint8_t pin() const;
  uint8_t pinValue() const;
  int updateConfig(const RelayConfig& config);
  std::shared_ptr<RelayConfig> getConfig() const;
  std::shared_ptr<RelayStatus> getStatus() const;
  std::shared_ptr<TimerTask> getRunTask() const;

 private:
  std::shared_ptr<RelayConfig> pConfig;
  std::shared_ptr<RelayStatus> pStatus;
  ASimpleTimer rTimer{"replay"};
  int runTimerId = -1;
  int checkTimerId = -1;
  int stopTimerId = -1;
  CALLBACK_FUNC callback;
  void check();
  void resetTimer();
};

#endif