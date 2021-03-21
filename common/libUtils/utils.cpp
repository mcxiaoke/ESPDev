#include "utils.h"

static time_t upTimestamp = 0;  // in seconds

std::vector<std::tuple<String, size_t>> listFiles(const char* dest) {
  std::vector<std::tuple<String, size_t>> output;
//   Serial.println(F("[System] SPIFFS Files:"));
#if defined(ESP8266)

  Dir f = SPIFFS.openDir(dest);
  while (f.next()) {
    // Serial.printf("[File] %s (%d bytes)\n", f.fileName().c_str(),
    // f.fileSize());
    output.push_back(std::make_tuple(f.fileName(), f.fileSize()));
  }

#elif defined(ESP32)
  File root = SPIFFS.open(dest);
  if (root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      Serial.printf("[File] %s (%d bytes)\n", f.name(), f.size());
      output.push_back(std::make_tuple(f.name(), f.size()));
      f = root.openNextFile();
    }
  }

#endif
  return output;
}

std::vector<std::tuple<String, size_t>> listLogs() {
  return listFiles("/logs/");
}

void fsCheck() {
#if defined(ESP8266)
  if (!SPIFFS.begin()) {
#elif defined(ESP32)
  if (!SPIFFS.begin(true)) {
#endif
    PLOGN(F("[Core] File System failed."));
  } else {
    PLOGN(F("[Core] File System ok."));
    FSInfo info;
    SPIFFS.info(info);
    PLOGNF("[Core] Free Space: %dK/%dK",
           (info.totalBytes - info.usedBytes) / 1000, info.totalBytes / 1000);
  }
}

String getUDID() {
  // ESP.getChipId();
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac.substring(mac.length() / 2);
}

String getHostName() {
#if defined(ESP32)
  String hn = "ESP32";
#elif defined(ESP8266)
  String hn = "ESP8266";
#endif
  return hn + "-" + getUDID();
}

String getMD5(uint8_t* data, uint16_t len) {
  MD5Builder md5;
  md5.begin();
  md5.add(data, len);
  md5.calculate();
  return md5.toString();
}

String getMD5(const char* data) {
  MD5Builder md5;
  md5.begin();
  md5.add(data);
  md5.calculate();
  return md5.toString();
}

String getMD5(const String& data) { return getMD5(data.c_str()); }

void showESP(const char* extra) {
#if defined(ESP8266)
  Serial.printf("[Core] Heap: %d/%d %s\n", ESP.getFreeContStack(),
                ESP.getFreeHeap(), extra);
#elif defined(ESP32)
  Serial.printf("[Core] Heap: %d %s\n", ESP.getFreeHeap(), extra);
#endif
}

String logFileName(const String& suffix) {
  String fileName = "/logs/log-";
  fileName += DateTime.format("%Y%m");
  if (suffix.length() > 0) {
    fileName += "-";
    fileName += suffix;
  }
  fileName += ".txt";
  return fileName;
}

size_t fileLog(const String& text, const String& path, bool appendDate) {
  String message = "";
#ifdef DEBUG
  message += "[D]";
#endif
  if (appendDate) {
    message += "[";
    message += dateTimeString();
    message += "] ";
  }
  message += text;
  PLOGN(message);
  size_t c = writeLine(path, message);
  return c;
}

bool trimLogFile(String fileName) {
  if (SPIFFS.exists(fileName)) {
    File file = SPIFFS.open(fileName, "r");
    if (file.size() > 100 * 1024) {
      String newName = fileName + ".old";
      SPIFFS.remove(newName);
      if (SPIFFS.rename(fileName.c_str(), newName.c_str())) {
        return true;
      }
    }
  }
  return false;
}

size_t writeLine(const String& path, const String& line) {
  return writeFile(path, line + "\n", true);
}

size_t writeFile(const String& path, const String& content, bool append) {
  File f = SPIFFS.open(path, append ? "a" : "w");
  if (!f) {
    return -1;
  }
  size_t c = f.print(content);
  f.close();
  return c;
}
String readFile(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return "";
  }
  String s = f.readString();
  f.close();
  return s;
}

time_t getTimestamp() { return DateTime.getTime(); }

time_t getBootTime() { return upTimestamp; }

String dateString() { return DateTime.format(DateFormatter::DATE_ONLY); }

String dateTimeString() { return DateTime.toString(); }

String timeString() { return DateTime.format(DateFormatter::TIME_ONLY); }
