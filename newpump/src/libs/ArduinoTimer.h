/*
 * SimpleTimer.h
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
 *
 */

#ifndef _ARDUINO_TIMER_H_
#define _ARDUINO_TIMER_H_

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#ifndef __AVR__
#include <functional>
#endif  // __AVR__

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include "../ext/utility.hpp"
#include "utils.h"

#ifndef __AVR__
typedef std::function<void(void)> timer_callback_func;
#else
typedef void (*timer_callback_func)();
#endif  // __AVR__

struct TimerTask {
  unsigned long interval;
  timer_callback_func action;
  int maxNumRuns;
  String name;
  bool enabled;
  int numRuns;
  int runType;
  unsigned long prevMillis;
  unsigned long offset;
  int id;
  bool debug;

  static int idCounter;

  TimerTask(unsigned long interval,
            timer_callback_func action,
            int maxNumRuns,
            String name = "task",
            bool debug = false);
  ~TimerTask();
};

class ArduinoTimer {
 public:
  // setTimer() constants
  constexpr static int RUN_FOREVER = INT_MAX / 10;  // 6 years
  constexpr static int RUN_ONCE = 1;

  // constructor
  explicit ArduinoTimer(const char* _name = "default", bool debugMode = false);

  // debug mode
  void setDebug(bool debug);

  // clear timers
  void reset();

  // this function must be called inside loop()
  void run();

  // call function f every d milliseconds
  int setInterval(unsigned long interval,
                  timer_callback_func action,
                  const String name = "task",
                  bool debug = false);

  // call function f once after d milliseconds
  int setTimeout(unsigned long interval,
                 timer_callback_func action,
                 const String name = "task",
                 bool debug = false);

  // call function f every d milliseconds for n times
  int setTimer(unsigned long interval,
               timer_callback_func action,
               int numRuns,
               const String name = "task",
               bool debug = false);

  // return timer task struct
  TimerTask* getTask(int taskId);

  // destroy the specified timer
  void deleteTimer(int taskId);

  // restart the specified timer
  void restartTimer(int taskId);

  // returns true if the specified timer is enabled
  bool isEnabled(int taskId);

  // enables the specified timer
  void enable(int taskId);

  // disables the specified timer
  void disable(int taskId);

  // enables the specified timer if it's currently disabled,
  // and vice-versa
  void toggle(int taskId);

  // return millis timer interval
  unsigned long getInterval(int taskId);

  // return elapsed after timer last run
  unsigned long getElapsed(int taskId);

  // return millis timer last run at
  unsigned long getPrevMs(int taskId);

  // return duration before timer next run
  unsigned long getRemain(int taskId);

  // return description of timer
  String getDescription(int taskId);

 private:
  // timer name
  const char* name;
  // debug mode flag
  bool debugMode;
  // task array
  std::vector<std::unique_ptr<TimerTask> > tasks;
};

#endif
