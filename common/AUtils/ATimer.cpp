#include "ATimer.h"

// deferred call constants
static constexpr int DEFCALL_DONTRUN = 0;  // don't call the callback function
static constexpr int DEFCALL_RUNONLY =
    1;  // call the callback function but don't delete the timer
static constexpr int DEFCALL_RUNANDDEL =
    2;  // call the callback function and delete the timer
static constexpr int DEFCALL_DELETEONLY = 3;

static int _taskId = 0;
static int generateId() { return ++_taskId; }

TimerTask::TimerTask(unsigned long interval, timer_callback_func action,
                     int maxNumRuns, String name, bool debug)
    : interval(interval),
      action(action),
      maxNumRuns(maxNumRuns),
      name(name),
      enabled(true),
      numRuns(0),
      runType(DEFCALL_DONTRUN),
      startMillis(millis()),
      prevMillis(0),
      offset(0),
      id(generateId()),
      debug(debug) {
  if (debug) {
    Serial.printf("TimerTask(%s,%d,%lu, %lu)\n", name.c_str(), id, startMillis,
                  interval);
  }
}

TimerTask::~TimerTask() {
  if (debug) {
    Serial.printf("~TimerTask(%s,%d,%lu,%lu,%d)\n", name.c_str(), id,
                  startMillis, interval, numRuns);
  }
  action = nullptr;
  name = "";
}

static inline unsigned long elapsed() { return millis(); }

ASimpleTimer::ASimpleTimer(const char* name, bool debugMode)
    : name(name), debugMode(debugMode) {
  tasks.reserve(5);
  reset();
}

void ASimpleTimer::setDebug(bool debug) { debugMode = debug; }

void ASimpleTimer::reset() {
  if (debugMode) {
    Serial.printf("[Timer-%s].reset()\n", name);
  }
  tasks.clear();
}

void ASimpleTimer::loop() {
  auto current_millis = elapsed();
  std::vector<int> toDelete;
  for (auto& task : tasks) {
    yield();
    if (current_millis - task->offset - task->prevMillis < task->interval) {
      continue;
    }
    task->runType = DEFCALL_DONTRUN;
    task->prevMillis += task->interval;
    if (task->enabled) {
      if (task->numRuns < task->maxNumRuns) {
        task->runType = DEFCALL_RUNONLY;
        task->numRuns++;
        // after last run, delete task
        if (task->numRuns >= task->maxNumRuns) {
          task->runType = DEFCALL_RUNANDDEL;
        }
      } else {
        task->runType = DEFCALL_DELETEONLY;
      }

      if (debugMode || task->debug) {
        Serial.printf(
            "[Timer-%s][%s][%d] num=%d max=%d int=%lu prev=%lu cur=%lu run\n",
            name, task->name.c_str(), task->id, task->numRuns, task->maxNumRuns,
            task->interval / 1000, task->prevMillis / 1000,
            current_millis / 1000);
      }

      if (task->action != nullptr) {
        switch (task->runType) {
          case DEFCALL_RUNONLY:
            task->action();
            break;

          case DEFCALL_RUNANDDEL:
            task->action();
            // !do not erase item in loop, will crash
            // delay execute task delete
            toDelete.push_back(task->id);
            break;
          case DEFCALL_DELETEONLY:
            toDelete.push_back(task->id);
            break;
          default:
            break;
        }
      }
    }
  }
  if (toDelete.size() > 0) {
    if (debugMode) {
      Serial.printf("[Timer-%s] after run: toDelete size=%d\n", name,
                    toDelete.size());
    }
    for (auto i : toDelete) {
      yield();
      deleteTimer(i);
    }
    toDelete.clear();
  }
}

int ASimpleTimer::setTimer(unsigned long interval, timer_callback_func action,
                           int numRuns, const String _name, bool debug) {
  if (action == nullptr) {
    return -1;
  }
  std::shared_ptr<TimerTask> task(
      new TimerTask(interval, action, numRuns, _name));
  int id = task->id;
  task->offset = elapsed();
  if (debugMode) {
    Serial.printf("Timer<%s>(%d/%d/%lu) %s added\n", name, task->id,
                  tasks.size(), task->interval, task->name.c_str());
  }
  tasks.push_back(std::move(task));
  return id;
}

int ASimpleTimer::setInterval(unsigned long interval,
                              timer_callback_func action, const String name,
                              bool debug) {
  return setTimer(interval, action, RUN_FOREVER, name, debug);
}

int ASimpleTimer::setTimeout(unsigned long interval, timer_callback_func action,
                             const String name, bool debug) {
  return setTimer(interval, action, RUN_ONCE, name, debug);
}

std::shared_ptr<TimerTask> ASimpleTimer::getTask(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task;
    }
  }
  return nullptr;
}

void ASimpleTimer::deleteTimer(int taskId) {
  if (tasks.empty()) {
    return;
  }
  tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                             [taskId](const std::shared_ptr<TimerTask> t) {
                               return t->id == taskId;
                             }),
              tasks.end());
}

void ASimpleTimer::restartTimer(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->prevMillis = elapsed();
    }
  }
}

bool ASimpleTimer::isEnabled(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->enabled;
    }
  }
  return false;
  // or using std::find_if
  //   auto it = std::find_if(tasks.begin(), tasks.end(),
  //                          [taskId](const unique_ptr<TimerTask>& t) {
  //                            if (t->id == taskId)
  //                              return true;
  //                            return false;
  //                          });
  //   return it != tasks.end() && (*it)->enabled;
}

void ASimpleTimer::enable(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = true;
    }
  }
}

void ASimpleTimer::disable(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = false;
    }
  }
}

void ASimpleTimer::toggle(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = !task->enabled;
    }
  }
}

unsigned long ASimpleTimer::getInterval(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->interval;
    }
  }
  return 2147483647L;
}

unsigned long ASimpleTimer::getElapsed(int taskId) const {
  return millis() - getPrevMs(taskId);
}

unsigned long ASimpleTimer::getPrevMs(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->prevMillis;
    }
  }
  return 2147483647L;
}

unsigned long ASimpleTimer::getRemain(int taskId) const {
  return getPrevMs(taskId) + getInterval(taskId) - millis();
}

String ASimpleTimer::getDescription(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->name;
    }
  }
  return "";
}

ASimpleTimer Timer{"default"};