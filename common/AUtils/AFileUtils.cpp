#include <AFileUtils.h>

using namespace std;

vector<tuple<String, size_t>> listFiles(const char* dest) {
  vector<tuple<String, size_t>> output;
//   Serial.println(F("[System] FileFS Files:"));
#if defined(ESP8266)

  Dir f = FileFS.openDir(dest);
  while (f.next()) {
    // Serial.printf("[File] %s (%d bytes)\n", f.fileName().c_str(),
    // f.fileSize());
    if (f.fileName().length() > 0 && f.fileSize() > 0) {
      output.push_back(make_tuple(f.fileName(), f.fileSize()));
    }
  }

#elif defined(ESP32)
  File root = FileFS.open(dest);
  if (root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      // Serial.printf("[File] %s (%d bytes)\n", f.name(), f.size());
      if (strlen(f.name()) > 0 && f.size() > 0) {
        output.push_back(make_tuple(f.name(), f.size()));
      }
      f = root.openNextFile();
    }
  }

#endif
  return output;
}

void checkFileSystem() {
#if defined(ESP8266)
  if (!FileFS.begin()) {
#elif defined(ESP32)
  if (!FileFS.begin(true)) {
#endif
    Serial.println("[Core] File System failed.");
  } else {
    Serial.println("[Core] File System OK.");
    // #if defined(ESP8266)
    //     FSInfo info;
    //     FileFS.info(info);
    //     LOGNF("[Core] Free Space: %dK/%dK",
    //           (info.totalBytes - info.usedBytes) / 1024, info.totalBytes /
    //           1024);
    // #elif defined(ESP32)
    //     LOGNF("[Core] Free Space: %dK/%dK",
    //           (FileFS.totalBytes() - FileFS.usedBytes()) / 1024,
    //           FileFS.totalBytes() / 1024);
    // #endif
  }
}

size_t writeLine(const String& path, const String& line) {
  return writeFile(path, line + "\n", true);
}

size_t writeFile(const String& path, const String& content, bool append) {
  File f = FileFS.open(path, append ? "a" : "w");
  if (!f) {
    return -1;
  }
  size_t c = f.print(content);
  f.close();
  return c;
}
String readFile(const String& path) {
  File f = FileFS.open(path, "r");
  if (!f) {
    return "";
  }
  String s = f.readString();
  f.close();
  return s;
}
