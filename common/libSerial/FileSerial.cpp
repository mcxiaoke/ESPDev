#include "FileSerial.h"

File FileSerialClass::ensureFile() {
  if (!this->fp.available()) {
    this->fp.close();
    this->fp = FileFS.open(this->path, "a");
  }
  return this->fp;
}

void FileSerialClass::setup() {
  this->end();
  // FileFS.remove(FILE_SERIAL_NAME);
  File f = FileFS.open(this->path, "w");
  f.write('\n');
  f.close();
}

void FileSerialClass::end() {
  if (this->fp) {
    this->fp.close();
  }
};

int FileSerialClass::available() { return this->fp.available(); }

int FileSerialClass::peek() { return this->fp.peek(); }
int FileSerialClass::read() { return this->fp.read(); }
void FileSerialClass::flush() { this->fp.flush(); }
size_t FileSerialClass::write(uint8_t c) { return this->fp.write(c); }
size_t FileSerialClass::print(char c) { return this->fp.print(c); }
size_t FileSerialClass::print(int c) { return this->fp.print(c); }
size_t FileSerialClass::print(unsigned int c) { return this->fp.print(c); }
size_t FileSerialClass::print(long c) { return this->fp.print(c); }
size_t FileSerialClass::print(unsigned long c) { return this->fp.print(c); }

size_t FileSerialClass::print(const char* s) { return this->print(String(s)); }
size_t FileSerialClass::println(const char* s) {
  return this->println(String(s));
}

size_t FileSerialClass::print(const String& s) {
  this->ensureFile();
  size_t n = this->fp.print(buildMessage(s));
  this->end();
  return n;
}
size_t FileSerialClass::println(const String& s) {
  this->ensureFile();
  size_t n = this->fp.println(buildMessage(s));
  this->end();
  return n;
}

FileSerialClass::FileSerialClass() {}

FileSerialClass::~FileSerialClass() {
  if (this->fp) {
    this->fp.close();
  }
}

FileSerialClass FileSerial;