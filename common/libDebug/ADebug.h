#ifndef __MCX_LIBSERIAL_ADEBUG_HEADER__
#define __MCX_LIBSERIAL_ADEBUG_HEADER__

#include <DevDebug.h>
#include <ALogger.h>

#include <ext/format.hpp>

// print logs only on debug mode
// #if defined(DEBUG) || defined(EANBLE_LOGGING)
// #define LOG(...) _log(__VA_ARGS__)
// #define LOG(...) _log(__VA_ARGS__)
#define LOGN(...) _logn(__VA_ARGS__)
#define LOGF(...) _logf(__VA_ARGS__)
#define ULOGN(...) _ulog(__VA_ARGS__)
#define ULOGF(...) _ulogf(__VA_ARGS__)
// #else
// #define LOG(...)
// #define LOGN(...)
// #define LOGF(...)
// #define LOGNF(...)
// #endif

// https://pabloariasal.github.io/2018/06/26/std-variant/
// https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.4.0/com.ibm.zos.v2r4.cbclx01/variadic_templates.htm
// https://www.cnblogs.com/qicosmos/p/4309835.html
// https://www.cnblogs.com/qicosmos/p/4325949.html
// https://florianjw.de/en/variadic_templates.html
// https://stackoverflow.com/questions/7124969

template <typename Arg>
void _log(Arg const& arg) {
  auto s = ext::format::ArgConvert(arg);
  ALogger.log(s);
}

template <typename Arg>
void _logn(Arg const& arg) {
  auto s = ext::format::ArgConvert(arg);
  ALogger.logn(s);
}

// must have this for zero args
template <typename... Args>
void _logn() {
  _log('\n');
}

// accept multi args, like python print
// template <typename Head, typename... Args>
// void _log(Head const& head, Args const&... args) {
//   int size = sizeof...(args);
//   _log(head);
//   if (size == 0) {
//     return;
//   } else {
//     _log(" ");
//   }
//   _log(args...);
// }

// accept multi args, like python print
// template <typename... Args>
// void _logn(Args const&... args) {
//   _log(args...);
//   _log('\n');
// }

template <typename... Args>
void _logf(char const* const format, Args const&... args) {
  _log(ext::format::strFormat(format, args...));
}

/**
 *  http://www.cplusplus.com/reference/cstdio/printf/
specifier	Output	Example
d or i	Signed decimal integer	392
u	Unsigned decimal integer	7235
o	Unsigned octal	610
x	Unsigned hexadecimal integer	7fa
X	Unsigned hexadecimal integer (uppercase)	7FA
f	Decimal floating point, lowercase	392.65
F	Decimal floating point, uppercase	392.65
e	Scientific notation (mantissa/exponent), lowercase	3.9265e+2
E	Scientific notation (mantissa/exponent), uppercase	3.9265E+2
g	Use the shortest representation: %e or %f	392.65
G	Use the shortest representation: %E or %F	392.65
a	Hexadecimal floating point, lowercase	-0xc.90fep-2
A	Hexadecimal floating point, uppercase	-0XC.90FEP-2
c	Character	a
s	String of characters	sample
p	Pointer address	b8000000
n	Nothing printed.
The corresponding argument must be a pointer to a signed int.
The number of characters written so far is stored in the pointed location.
%	A % followed by another % character will write a single % to the stream.
%

**/

template <typename Arg>
void _ulog() {}

template <typename Arg>
void _ulog(Arg const& arg) {
  auto s = ext::format::ArgConvert(arg);
  ALogger.ulog(s);
}

template <typename... Args>
void _ulogf(char const* const format, Args const&... args) {
  _ulog(ext::format::strFormat(format, args...));
}

#endif /* __MCX_LIBSERIAL_ADEBUG_HEADER__ */
