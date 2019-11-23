/*
 * SimpleTimer.cpp
 *
 * SimpleTimer - A timer library for Arduino.
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 * This library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser
 * General Public License along with this library; if not,
 * write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ArduinoTimer.h"

// Select time function:
// static inline unsigned long elapsed() { return micros(); }
static inline unsigned long elapsed() {
  return millis();
}

ArduinoTimer::ArduinoTimer(const char* _name) {
  name = _name;
  reset();
}

void ArduinoTimer::setDebug(bool debug) {
  debugMode = debug;
}

void ArduinoTimer::setBootTime(time_t timestamp) {
  bootTime = timestamp;
}

void ArduinoTimer::reset() {
  Serial.printf("Timer<%s>.reset()", name);
  for (int i = 0; i < MAX_TIMERS; i++) {
    deleteTimer(i);
  }
  numTimers = 0;
}

void ArduinoTimer::run() {
  int i;
  auto current_millis = elapsed();

  for (i = 0; i < MAX_TIMERS; i++) {
    toBeCalled[i] = DEFCALL_DONTRUN;

    // no callback == no timer, i.e. jump over empty slots
    if (callbacks[i]) {
      // is it time to process this timer ?
      // see
      // http://arduino.cc/forum/index.php/topic,124048.msg932592.html#msg932592

      if (current_millis - prev_millis[i] >= delays[i]) {
        // update time
        // prev_millis[i] = current_millis;
        prev_millis[i] += delays[i];

        // check if the timer callback has to be executed
        if (enabled[i]) {
          // "run forever" timers must always be executed
          if (maxNumRuns[i] == RUN_FOREVER) {
            toBeCalled[i] = DEFCALL_RUNONLY;
          }
          // other timers get executed the specified number of times
          else if (numRuns[i] < maxNumRuns[i]) {
            toBeCalled[i] = DEFCALL_RUNONLY;
            numRuns[i]++;

            // after the last run, delete the timer
            if (numRuns[i] >= maxNumRuns[i]) {
              toBeCalled[i] = DEFCALL_RUNANDDEL;
            }
          }

          if (debugMode) {
            Serial.printf("Timer<%s>(%d) %s run at %lu (%d)\n", name, i,
                          descriptions[i].c_str(), current_millis / 1000UL,
                          toBeCalled[i]);
          }
        }
      }
    }
  }

  for (i = 0; i < MAX_TIMERS; i++) {
    switch (toBeCalled[i]) {
      case DEFCALL_DONTRUN:
        break;

      case DEFCALL_RUNONLY:
        callbacks[i]();
        break;

      case DEFCALL_RUNANDDEL:
        callbacks[i]();
        deleteTimer(i);
        break;
    }
  }
}

// find the first available slot
// return -1 if none found
int ArduinoTimer::findFirstFreeSlot() {
  int i;

  // all slots are used
  if (numTimers >= MAX_TIMERS) {
    return -1;
  }

  // return the first slot with no callback (i.e. free)
  for (i = 0; i < MAX_TIMERS; i++) {
    if (callbacks[i] == 0) {
      return i;
    }
  }

  // no free slots found
  return -1;
}

int ArduinoTimer::setTimer(unsigned long d,
                           timer_callback_func f,
                           int n,
                           const String s) {
  int freeTimer = findFirstFreeSlot();
  if (freeTimer < 0) {
    return -1;
  }

  if (f == NULL) {
    return -1;
  }

  delays[freeTimer] = d;
  callbacks[freeTimer] = f;
  descriptions[freeTimer] = s;
  maxNumRuns[freeTimer] = n;
  enabled[freeTimer] = true;
  prev_millis[freeTimer] = elapsed();

  numTimers++;

  if (debugMode) {
    Serial.printf("Timer<%s>(%d) %s added\n", name, freeTimer,
                  descriptions[freeTimer].c_str());
  }

  return freeTimer;
}

int ArduinoTimer::setInterval(unsigned long d,
                              timer_callback_func f,
                              const String s) {
  return setTimer(d, f, RUN_FOREVER, s);
}

int ArduinoTimer::setTimeout(unsigned long d,
                             timer_callback_func f,
                             const String s) {
  return setTimer(d, f, RUN_ONCE, s);
}

void ArduinoTimer::deleteTimer(int timerId) {
  if (timerId >= MAX_TIMERS) {
    return;
  }

  // nothing to delete if no timers are in use
  if (numTimers == 0) {
    return;
  }

  // don't decrease the number of timers if the
  // specified slot is already empty
  if (callbacks[timerId] != NULL) {
    if (debugMode) {
      Serial.printf("Timer<%s>(%d) %s deleted\n", name, timerId,
                    descriptions[timerId].c_str());
    }
    callbacks[timerId] = 0;
    descriptions[timerId] = "";
    enabled[timerId] = false;
    toBeCalled[timerId] = DEFCALL_DONTRUN;
    delays[timerId] = 0;
    numRuns[timerId] = 0;

    // update number of timers
    numTimers--;
  }
}

// function contributed by code@rowansimms.com
void ArduinoTimer::restartTimer(int numTimer) {
  if (numTimer >= MAX_TIMERS) {
    return;
  }

  prev_millis[numTimer] = elapsed();
}

bool ArduinoTimer::isEnabled(int numTimer) {
  if (numTimer >= MAX_TIMERS) {
    return false;
  }

  return enabled[numTimer];
}

void ArduinoTimer::enable(int numTimer) {
  if (numTimer >= MAX_TIMERS) {
    return;
  }

  enabled[numTimer] = true;
}

void ArduinoTimer::disable(int numTimer) {
  if (numTimer >= MAX_TIMERS) {
    return;
  }

  enabled[numTimer] = false;
}

void ArduinoTimer::toggle(int numTimer) {
  if (numTimer >= MAX_TIMERS) {
    return;
  }

  enabled[numTimer] = !enabled[numTimer];
}

int ArduinoTimer::getNumTimers() {
  return numTimers;
}

unsigned long ArduinoTimer::getInterval(int numTimer) {
  return delays[numTimer];
}

unsigned long ArduinoTimer::getElapsed(int numTimer) {
  return millis() - prev_millis[numTimer];
}

unsigned long ArduinoTimer::getPrevMs(int numTimer) {
  return prev_millis[numTimer];
}

unsigned long ArduinoTimer::getRemain(int numTimer) {
  return prev_millis[numTimer] + delays[numTimer] - millis();
}

String ArduinoTimer::getDescription(int numTimer) {
  return descriptions[numTimer];
}
