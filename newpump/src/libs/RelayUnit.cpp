#include "RelayUnit.h"

void RelayStatus::onStart() {
  lastStart = millis();
}

void RelayStatus::onStop() {
  lastStop = millis();
  if (lastStart > 0) {
    lastElapsed = lastStop - lastStart;
    totalElapsed += lastElapsed;
  }
}

RelayUnit::RelayUnit() {
  // nothing
}

RelayUnit::RelayUnit(const RelayConfig& cfg)
    : pConfig(std::make_shared<RelayConfig>(cfg)),
      pStatus(std::make_shared<RelayStatus>()) {}

void RelayUnit::begin(const RelayConfig& cfg) {
  pConfig = std::make_shared<RelayConfig>(cfg);
  Serial.printf("RelayUnit::begin name=%s interval=%lu, duration=%lu\n",
                pConfig->name.c_str(), pConfig->interval, pConfig->duration);
  pinMode(pConfig->pin, OUTPUT);
  pStatus = std::make_shared<RelayStatus>();
  pStatus->setupAt = millis();
  resetTimer();
}

void RelayUnit::run() {
  timer.run();
}

bool RelayUnit::start() {
  LOGN("start");
  if (isOn()) {
    return false;
  }
  pStatus->onStart();
  digitalWrite(pConfig->pin, HIGH);
  auto stopFunc = std::bind(&RelayUnit::stop, this);
  stopTimerId = timer.setTimeout(pConfig->duration, stopFunc, "stop");
  if (callback) {
    callback(RelayEvent::STARTED, 0);
  }
  return true;
}

bool RelayUnit::stop() {
  if (!isOn()) {
    return false;
  }
  pStatus->onStop();
  digitalWrite(pConfig->pin, LOW);
  if (callback) {
    callback(RelayEvent::STOPPED, 0);
  }
  return true;
}

void RelayUnit::check() {
  if (isOn() && pStatus->lastStart > 0 &&
      (millis() - pStatus->lastStart) / 1000 >= pConfig->duration) {
    LOGN(F("Stopped by watchdog"));
    stop();
  }
}

void RelayUnit::resetTimer() {
  pStatus->timerResetAt = millis();
  timer.setDebug(true);
  timer.reset();
  auto startFunc = std::bind(&RelayUnit::start, this);
  runTimerId = timer.setInterval(pConfig->interval, startFunc, "start");
  auto checkFunc = std::bind(&RelayUnit::check, this);
  checkTimerId =
      timer.setInterval(pConfig->duration / 2 + 2000, checkFunc, "check");
}

bool RelayUnit::isOn() {
  return pinValue() == HIGH;
}

bool RelayUnit::isEnabled() {
  return timer.isEnabled(runTimerId);
}

void RelayUnit::setEnabled(bool enable) {
  pStatus->enabled = enable;
  if (enable) {
    timer.enable(runTimerId);
    timer.enable(checkTimerId);
    if (callback) {
      callback(RelayEvent::ENABLED, 0);
    }
  } else {
    timer.disable(runTimerId);
    timer.disable(checkTimerId);
    if (callback) {
      callback(RelayEvent::DISABLED, 0);
    }
  }
}

void RelayUnit::setCallback(CALLBACK_FUNC cb) {
  callback = cb;
}

uint8_t RelayUnit::pin() {
  return pConfig->pin;
}

uint8_t RelayUnit::pinValue() {
  return digitalRead(pConfig->pin);
}

int RelayUnit::updateConfig(const RelayConfig& config) {
  int changed = 0;
  if (pConfig->pin != config.pin) {
    pConfig->pin = config.pin;
    changed++;
  }
  if (pConfig->interval != config.interval) {
    pConfig->interval = config.interval;
    changed++;
  }
  if (pConfig->duration != config.duration) {
    pConfig->duration = config.duration;
    changed++;
  }
  if (changed > 0) {
    // ensure relay stopped
    stop();
    resetTimer();
    if (callback) {
      callback(RelayEvent::RESET, 0);
    }
  }
  return changed;
}

std::shared_ptr<RelayConfig> RelayUnit::getConfig() {
  return pConfig;
}

std::shared_ptr<RelayStatus> RelayUnit::getStatus() {
  return pStatus;
}