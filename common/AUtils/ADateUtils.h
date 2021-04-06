#ifndef __MCX_AUTILS_ADATEUTILS_HEADER__
#define __MCX_AUTILS_ADATEUTILS_HEADER__

#include <ESPDateTime.h>

inline time_t getTimestamp() { return DateTime.getTime(); }

inline time_t getBootTime() { return DateTime.getBootTime(); }

inline String dateString() { return DateTime.format(DateFormatter::DATE_ONLY); }

inline String dateTimeString() { return DateTime.toString(); }

inline String timeString() { return DateTime.format(DateFormatter::TIME_ONLY); }

#endif /* __MCX_AUTILS_ADATEUTILS_HEADER__ */
