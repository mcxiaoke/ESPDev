#include "RelayUnit.h"

static void updateStatusOnStart(const std::shared_ptr<RelayStatus>& st) {
  st->lastStart = millis();
}

static void updateStatusOnSop(const std::shared_ptr<RelayStatus>& st) {
  st->lastStop = millis();
  if (st->lastStart > 0) {
    st->lastElapsed = st->lastStop - st->lastStart;
    st->totalElapsed += st->lastElapsed;
  }
}

std::string RelayConfig::toString() const {
  return ext::strFormat2("Config(%s[%d],%lus/%lus)", name, pin, interval / 1000,
                         duration / 1000);
}

void RelayStatus::reset() {
  setupAt = 0;
  timerResetAt = millis();
  lastStart = 0;
  lastStop = 0;
  lastElapsed = 0;
  totalElapsed = 0;
}

std::string RelayStatus::toString() const {
  return ext::strFormat2(
      "Status(setup=%lu,reset=%lu,start=%lu,stop=%lu,"
      "elapsed=%lu/%lu)",
      setupAt, timerResetAt, lastStart, lastStop, lastElapsed, totalElapsed);
}

RelayUnit::RelayUnit()
    : pConfig(nullptr), pStatus(std::make_shared<RelayStatus>()) {}

RelayUnit::RelayUnit(const RelayConfig& cfg)
    : pConfig(std::make_shared<RelayConfig>(cfg)),
      pStatus(std::make_shared<RelayStatus>()) {}

void RelayUnit::begin(const RelayConfig& cfg) {
  LOGF("[Relay] With %s\n", cfg.toString());
  DLOG();
  pinMode(cfg.pin, OUTPUT);
  pConfig = std::make_shared<RelayConfig>(cfg);
  pStatus->setupAt = millis();
  reset();
  // LOGN(pConfig->toString().c_str());
  // LOGN(pStatus->toString().c_str());
}

void RelayUnit::run() { rTimer.loop(); }

bool RelayUnit::start() {
  DLOG();
  if (isOn()) {
    return false;
  }
  // no water check
  if (pStatus->totalElapsed > pConfig->duration * 10) {
    // 30s * 10 = 300s
    return false;
  }
  LOGN("RelayUnit::start");
  updateStatusOnStart(pStatus);
  digitalWrite(pConfig->pin, HIGH);
  auto stopFunc = std::bind(&RelayUnit::stop, this);
  stopTimerId = rTimer.setTimeout(pConfig->duration, stopFunc, "relay_stop");
  if (callback) {
    callback(RelayEvent::Started, 0);
  }
  return true;
}

bool RelayUnit::stop() {
  DLOG();
  if (!isOn()) {
    return false;
  }
  LOGN("RelayUnit::stop");
  updateStatusOnSop(pStatus);
  digitalWrite(pConfig->pin, LOW);
  if (callback) {
    callback(RelayEvent::Stopped, 0);
  }
  return true;
}

void RelayUnit::check() {
  DLOG();
  // LOGN("RelayUnit::check");
  if (isOn() && pStatus->lastStart > 0 &&
      (millis() - pStatus->lastStart) / 1000 >= pConfig->duration) {
    LOGN("Stopped by watchdog");
    stop();
  }
}

void RelayUnit::resetTimer() {
  DLOG();
  // LOGN("RelayUnit::resetTimer");
  rTimer.reset();
  pStatus->reset();
  auto startFunc = std::bind(&RelayUnit::start, this);
  runTimerId = rTimer.setInterval(pConfig->interval, startFunc, "relay_start");
  // timer default disabled
  rTimer.disable(runTimerId);
  auto checkFunc = std::bind(&RelayUnit::check, this);
  checkTimerId = rTimer.setInterval(pConfig->duration / 2 + 2000, checkFunc,
                                    "relay_check");
}

bool RelayUnit::isOn() const { return pinValue() == HIGH; }

bool RelayUnit::isTimerEnabled() const { return rTimer.isEnabled(runTimerId); }

void RelayUnit::setTimerEnabled(bool enable) {
  DLOG();
  LOGF("RelayUnit::setEnabled:%s\n", enable ? "true" : "false");
  if (enable == rTimer.isEnabled(runTimerId)) {
    return;
  }
  stop();
  if (enable) {
    rTimer.enable(runTimerId);
    if (callback) {
      callback(RelayEvent::Enabled, 0);
    }
  } else {
    rTimer.disable(runTimerId);
    if (callback) {
      callback(RelayEvent::Disabled, 0);
    }
  }
}

void RelayUnit::setCallback(CALLBACK_FUNC cb) { callback = cb; }

uint8_t RelayUnit::pin() const { return pConfig->pin; }

uint8_t RelayUnit::pinValue() const { return digitalRead(pConfig->pin); }

int RelayUnit::updateConfig(const RelayConfig& config) {
  DLOG();
  LOGF("RelayUnit::updateConfig, old:%s\n", pConfig->toString().c_str());
  int changed = 0;
  if (config.pin > 0 && pConfig->pin != config.pin) {
    pConfig->pin = config.pin;
    changed++;
  }
  if (config.interval > 0 && pConfig->interval != config.interval) {
    pConfig->interval = config.interval;
    changed++;
  }
  if (config.duration > 0 && pConfig->duration != config.duration) {
    pConfig->duration = config.duration;
    changed++;
  }
  if (changed > 0) {
    LOGN("RelayUnit::updateConfig changed");
    // ensure relay stopped
    reset();
    if (callback) {
      callback(RelayEvent::ConfigChanged, 0);
    }
    LOGF("RelayUnit::updateConfig, new:%s\n", pConfig->toString().c_str());
  }
  return changed;
}

void RelayUnit::reset() {
  // LOGN("RelayUnit::reset");
  stop();
  resetTimer();
}

std::shared_ptr<RelayConfig> RelayUnit::getConfig() const { return pConfig; }

std::shared_ptr<RelayStatus> RelayUnit::getStatus() const { return pStatus; }

std::shared_ptr<TimerTask> RelayUnit::getRunTask() const {
  return rTimer.getTask(runTimerId);
}