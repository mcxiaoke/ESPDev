#include "AFileLogger.h"

File AFileLogger::ensureFile() {
  if (!this->fp.isFile()) {
    Serial.println("=== Reopen file ===");
    this->fp = FileFS.open(this->path, "a");
  }
  return this->fp;
}

void AFileLogger::setup() {
  ensureFile();
  this->fp.print("\n");
  this->fp.flush();
}

void AFileLogger::end() {
  auto now = millis();
  if (now - _lastCloseMs > 10 * 1000L) {
    _lastCloseMs = now;
    if (this->fp) {
      this->fp.close();
      Serial.println("=== Close file === " + this->fp);
    }
  } else {
    this->fp.flush();
  }
};

int AFileLogger::available() { return this->fp.available(); }

int AFileLogger::peek() { return this->fp.peek(); }
int AFileLogger::read() { return this->fp.read(); }
void AFileLogger::flush() { this->fp.flush(); }
size_t AFileLogger::write(uint8_t c) { return this->fp.write(c); }
size_t AFileLogger::print(char c) { return this->fp.print(c); }
size_t AFileLogger::print(int c) { return this->fp.print(c); }
size_t AFileLogger::print(unsigned int c) { return this->fp.print(c); }
size_t AFileLogger::print(long c) { return this->fp.print(c); }
size_t AFileLogger::print(unsigned long c) { return this->fp.print(c); }

size_t AFileLogger::print(const char* s) { return this->print(String(s)); }
size_t AFileLogger::println(const char* s) { return this->println(String(s)); }

size_t AFileLogger::print(const String& s) {
  this->ensureFile();
  size_t n = this->fp.print(buildMessage(s));
  this->end();
  return n;
}
size_t AFileLogger::println(const String& s) {
  this->ensureFile();
  size_t n = this->fp.println(buildMessage(s));
  this->end();
  return n;
}

AFileLogger::AFileLogger() {}

AFileLogger::~AFileLogger() {
  if (this->fp) {
    this->fp.close();
  }
}