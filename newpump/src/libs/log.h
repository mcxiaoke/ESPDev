#ifndef __SIMPLE_LOG_H_
#define __SIMPLE_LOG_H_

#include "../ext/format.hpp"
#include "compat.h"

#if defined(EANBLE_LOGGING) || defined(DEBUG_MODE)
#define LOG(...) _log(__VA_ARGS__)
#define LOGN(...) _logn(__VA_ARGS__)
#define LOGF(format, ...) _logf(format, __VA_ARGS__)
#else
#define LOG(...)
#define LOGN(...)
#define LOGF(...)
#endif

// cpp typesafe printf
template <typename... Args>
void _logf(char const* const format, Args const&... args) {
#if defined(EANBLE_LOGGING) || defined(DEBUG_MODE)
  Serial.print(ext::format::strFormat(format, args...).c_str());
#endif
}

template <typename Arg>
void _log(Arg const& arg) {
  Serial.print(ext::format::ArgConvert(arg));
}

template <typename Arg>
void _logn(Arg const& arg) {
  Serial.println(ext::format::ArgConvert(arg));
}

template <typename Head, typename... Args>
void _log(Head const& head, Args const&... args, const char& delimiter = ' ') {
  int size = sizeof...(args);
  _log(head);
  if (size == 0) {
    return;
  } else {
    _log(delimiter);
  }
  _log(args..., delimiter);
}

// accept multi args, like python print
template <typename... Args>
void _logn(Args const&... args, const char& delimiter = ' ') {
  _log(args..., delimiter);
  _log('\n');
}

void _logn();

#endif