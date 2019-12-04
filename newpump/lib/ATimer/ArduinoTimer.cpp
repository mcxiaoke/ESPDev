#include "ArduinoTimer.h"

// deferred call constants
static constexpr int DEFCALL_DONTRUN = 0;  // don't call the callback function
static constexpr int DEFCALL_RUNONLY =
    1;  // call the callback function but don't delete the timer
static constexpr int DEFCALL_RUNANDDEL =
    2;  // call the callback function and delete the timer
static constexpr int DEFCALL_DELETEONLY = 3;

static int _taskId = 0;
static int generateId() {
  return ++_taskId;
}

TimerTask::TimerTask(unsigned long interval,
                     timer_callback_func action,
                     int maxNumRuns,
                     String name,
                     bool debug)
    : interval(interval),
      action(action),
      maxNumRuns(maxNumRuns),
      name(name),
      enabled(true),
      numRuns(0),
      runType(DEFCALL_DONTRUN),
      prevMillis(0),
      offset(0),
      id(generateId()),
      debug(debug) {
  if (debug) {
    Serial.printf("TimerTask(%s,%d,%lu)\n", name.c_str(), id, interval);
  }
}

TimerTask::~TimerTask() {
  if (debug) {
    Serial.printf("~TimerTask(%s,%d,%lu,%d)\n", name.c_str(), id, interval,
                  numRuns);
  }
  action = nullptr;
  name = "";
}

static inline unsigned long elapsed() {
  return millis();
}

ArduinoTimer::ArduinoTimer(const char* name, bool debugMode)
    : name(name), debugMode(debugMode) {
  tasks.reserve(5);
  reset();
}

void ArduinoTimer::setDebug(bool debug) {
  debugMode = debug;
}

void ArduinoTimer::reset() {
  if (debugMode) {
    Serial.printf("[Timer-%s].reset()\n", name);
  }
  tasks.clear();
}

void ArduinoTimer::run() {
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
            "[Timer-%s][%s][%d] num=%d max=%d int=%lu prev=%lu cur=%lu\n", name,
            task->name.c_str(), task->id, task->numRuns, task->maxNumRuns,
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

int ArduinoTimer::setTimer(unsigned long interval,
                           timer_callback_func action,
                           int numRuns,
                           const String _name,
                           bool debug) {
  if (action == nullptr) {
    return -1;
  }
  std::unique_ptr<TimerTask> task(
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

int ArduinoTimer::setInterval(unsigned long interval,
                              timer_callback_func action,
                              const String name,
                              bool debug) {
  return setTimer(interval, action, RUN_FOREVER, name, debug);
}

int ArduinoTimer::setTimeout(unsigned long interval,
                             timer_callback_func action,
                             const String name,
                             bool debug) {
  return setTimer(interval, action, RUN_ONCE, name, debug);
}

TimerTask* ArduinoTimer::getTask(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task.get();
    }
  }
  return nullptr;
}

void ArduinoTimer::deleteTimer(int taskId) {
  if (tasks.empty()) {
    return;
  }
  tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                             [taskId](const std::unique_ptr<TimerTask>& t) {
                               return t->id == taskId;
                             }),
              tasks.end());
}

void ArduinoTimer::restartTimer(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->prevMillis = elapsed();
    }
  }
}

bool ArduinoTimer::isEnabled(int taskId) const {
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

void ArduinoTimer::enable(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = true;
    }
  }
}

void ArduinoTimer::disable(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = false;
    }
  }
}

void ArduinoTimer::toggle(int taskId) {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      task->enabled = !task->enabled;
    }
  }
}

unsigned long ArduinoTimer::getInterval(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->interval;
    }
  }
  return 2147483647L;
}

unsigned long ArduinoTimer::getElapsed(int taskId) const {
  return millis() - getPrevMs(taskId);
}

unsigned long ArduinoTimer::getPrevMs(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->prevMillis;
    }
  }
  return 2147483647L;
}

unsigned long ArduinoTimer::getRemain(int taskId) const {
  return getPrevMs(taskId) + getInterval(taskId) - millis();
}

String ArduinoTimer::getDescription(int taskId) const {
  for (auto& task : tasks) {
    if (task->id == taskId) {
      return task->name;
    }
  }
  return "";
}
