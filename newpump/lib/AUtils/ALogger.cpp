#include "ALogger.h"

#ifdef LOG_PRINT_TEST

static void cr() {
  Serial.println();
}

static void p(const char* str) {
  Serial.println(str);
}

#endif
