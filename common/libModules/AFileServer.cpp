#include "AFileServer.h"

// static constexpr const char* SAFE_MODE_IGNORE PROGMEM =
// "[FileServer] Safe Mode, ignore.";

static String getFilesHtml() {
  auto items = listFiles();
  String html = "<ul>";
  for (auto const& i : items) {
    html += "<li><a href='";
    html += std::get<0>(i);
    html += "' target='_blank' >";
    html += std::get<0>(i);
    html += " (";
    html += std::get<1>(i);
    html += " bytes)</a></li>\n";
  }
  html += "</ul>";
  return html;
}

void AFileServerClass::setup(std::shared_ptr<AsyncWebServer> server) {
  ULOGN("[FileServer] Add handler for /files");
  server->on("/files", [](AsyncWebServerRequest* request) {
    request->send(200, MIME_TEXT_HTML, getFilesHtml());
  });
  ULOGN("[FileServer] Add handler for static files");
  server->onNotFound([this](AsyncWebServerRequest* request) {
    if (!AFileServer.handle(request)) {
      String data = F("ERROR: NOT FOUND\nURI: ");
      data += request->url();
      data += "\n";
      request->send(404, MIME_TEXT_PLAIN, data);
    }
  });
}

bool AFileServerClass::begin() {
  LOGN(F("[FileServer] Setup File Server"));
  return true;
};

bool AFileServerClass::handle(AsyncWebServerRequest* request) {
  String path = request->url();
  ULOGN("[FileServer] Handling " + path);
  if (request->hasParam("delete")) {
    if (FileFS.exists(path)) {
      FileFS.remove(path);
      request->redirect("/");
      LOGF("[FileServer] File '%s' Deleted\n", path);
      return true;
    }
    return false;
  }
  if (path.endsWith("/")) path += "index.html";
  String contentType;
  if (path.endsWith(".htm") || path.endsWith(".html"))
    contentType = "text/html";
  else if (path.endsWith(".css"))
    contentType = "text/css";
  else if (path.endsWith(".js"))
    contentType = "application/javascript";
  else if (path.endsWith(".png"))
    contentType = "image/png";
  else if (path.endsWith(".gif"))
    contentType = "image/gif";
  else if (path.endsWith(".jpg"))
    contentType = "image/jpeg";
  else if (path.endsWith(".ico"))
    contentType = "image/x-icon";
  else if (path.endsWith(".xml"))
    contentType = "text/xml";
  else if (path.endsWith(".pdf"))
    contentType = "application/x-pdf";
  else if (path.endsWith(".zip"))
    contentType = "application/x-zip";
  else if (path.endsWith(".gz"))
    contentType = "application/x-gzip";
  else if (path.endsWith(".json"))
    contentType = "application/json";
  else
    contentType = "text/plain";

  // split filepath and extension
  String prefix = path, ext = "";
  int lastPeriod = path.lastIndexOf('.');
  if (lastPeriod >= 0) {
    prefix = path.substring(0, lastPeriod);
    ext = path.substring(lastPeriod);
  }

  // look for smaller versions of file
  // minified file, good (myscript.min.js)
  if (FileFS.exists(prefix + ".min" + ext)) path = prefix + ".min" + ext;
  // gzipped file, better (myscript.js.gz)
  // if (FileFS.exists(prefix + ext + ".gz")) path = prefix + ext + ".gz";
  // min and gzipped file, best (myscript.min.js.gz)
  // if (FileFS.exists(prefix + ".min" + ext + ".gz"))
  // path = prefix + ".min" + ext + ".gz";
  // if FileFS.exists
  if (FileFS.exists(path)) {
    ULOGF("[FileServer] Sending %s\n", path);

    AsyncWebServerResponse* response =
        request->beginResponse(FileFS, path, contentType);

    File file = FileFS.open(path, "r");
    if (request->hasParam("download", false))
      response->addHeader("Content-Disposition", " attachment;");
    if (path.indexOf("nocache") < 0)
      response->addHeader("Cache-Control", " max-age=14400");

    // optional alt arg (encoded url), server sends redirect to file on the web
    if (request->hasParam("alt", false)) {
      request->redirect(request->getParam("alt")->value());
    } else {
      // server sends file
      request->send(response);
    }
    file.close();
    return true;
  } else {
    ULOGF("[FileServer] Not Found %s\n", path);
  }
  return false;
}

AFileServerClass AFileServer;