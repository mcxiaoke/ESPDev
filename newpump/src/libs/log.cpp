#include "log.h"

#ifdef LOG_PRINT_TEST

static void cr() {
  Serial.println();
}

static void p(const char* str) {
  Serial.println(str);
}

static void _print_test_1() {
  int r = INT_MAX / 100;
  LOGF("Strings:\n");

  const char* s = "Hello";
  LOGF("\t[%10s]\n\t[%-10s]\n\t[%*s]\n\t[%-10.*s]\n\t[%-*.*s]\n", s, s, 10, s,
       4, s, 10, 4, s);

  LOGF("Characters:\t%c %%\n", 65);

  LOGF("Integers\n");
  LOGF("Decimal:\t%i %d %.6i %i %.0i %+i %u\n", 1, 2, 3, 0, 0, 4, -1);
  LOGF("Hexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
  LOGF("Octal:\t%o %#o %#o\n", 10, 10, 4);

  LOGF("Floating point\n");
  LOGF("Rounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.5);
  LOGF("Padding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
  LOGF("Scientific:\t%E %e\n", 1.5, 1.5);
  LOGF("Hexadecimal:\t%a %A\n", 1.5, 1.5);
  LOGF("Special values:\t0/0=%g 1/0=%g\n", 0.0 / 0.0, 1.0 / 0.0);

  LOGF("Variable width control:\n");
  LOGF("right-justified variable width: '%*c'\n", 5, 'x');
  LOGF("left-justified variable width : '%*c'\n", -5, 'x');
  LOGF("(the last printf printed %d characters)\n", r);

  // 定宽类型
  std::uint32_t val = std::numeric_limits<std::uint32_t>::max();
  LOGF("Largest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n", val, val);
}

static void _print_test_2() {
  char c = '@';
  int i = INT_MAX / 2;
  unsigned int i2 = 12345678;
  double d = 3.14159265;
  unsigned long u = ULONG_MAX / 2;
  const char* cs = "Hello, Chars!";
  std::string ss = "std::string text";
  String as = "Arduino String Class 2019";
  // test LOG
  p("====== LOG TEST 01 ======");
  LOG(c);
  cr();
  LOG(i);
  cr();
  LOG(d);
  cr();
  LOG(u);
  cr();
  LOG(cs);
  cr();
  LOG(ss);
  cr();
  LOG(as);
  cr();
  p("====== LOG TEST 02 ======");
  LOG(i, d, u, cs, ss, as, c);
  cr();
  LOG(i, cs, d, ss, u, as, c);
  cr();
  p("====== LOGN TEST 01 ======");
  LOGN(c);
  LOGN(i);
  LOGN(d);
  LOGN(u);
  LOGN(cs);
  LOGN(ss);
  LOGN(as);
  p("====== LOGN TEST 02 ======");
  LOGN(c, i, d, u, cs, ss, as);
  LOGN(c, i, cs, d, ss, u, as);
  p("====== LOGF TEST 01 ======");
  LOGF("Char=%c,Int=%d,Int=%x,Double=%f,Long=%lu\n", c, i, i2, d, u);
  LOGF("CharStr=%s,std::string=%s,ArduinoString=%s\n", cs, ss, as);
  LOGF("IntP=%p,CharStrP=%p,StdStrP=%p,ArdStrP=%p\n", i, cs, ss, as);
  p("====== LOGNF TEST 01 ======");
  LOGNF("Char=%c,Int=%d,Int=%x,Double=%f,Long=%lu", c, i, i2, d, u);
  LOGNF("CharStr=%s,std::string=%s,ArduinoString=%s", cs, ss, as);
}

#endif

void _log_test_all() {
#ifdef LOG_PRINT_TEST
  p("========================================");
  _print_test_1();
  _print_test_2();
  p("========================================");
#endif
}
