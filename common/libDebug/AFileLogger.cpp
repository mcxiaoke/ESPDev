#include "AFileLogger.h"

void AFileLogger::fpCheck() {
  if (!this->fp || !FileFS.exists(this->path)) {
    DLOG();
    this->fp = FileFS.open(this->path, "a");
  }
}

void AFileLogger::setup() {
  fpCheck();
  this->fp.print("\n");
  this->fp.flush();
}

void AFileLogger::clear() {
  this->fp.close();
  String newPath = this->path;
  newPath += ".old";
  if (FileFS.exists(newPath)) {
    FileFS.remove(newPath);
  }
  FileFS.rename(this->path, newPath);
  fpCheck();
}

void AFileLogger::end() {
  auto now = millis();
  if (now - _lastCloseMs > 10 * 1000L) {
    _lastCloseMs = now;
    if (this->fp) {
      DLOG();
      this->fp.close();
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
  this->fpCheck();
  size_t n = this->fp.print(buildMessage(s));
  this->end();
  return n;
}
size_t AFileLogger::println(const String& s) {
  this->fpCheck();
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