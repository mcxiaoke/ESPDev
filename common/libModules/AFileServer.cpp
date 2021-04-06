#include "AFileServer.h"

bool AFileServerClass::handle(AsyncWebServerRequest* request) {
  String path = request->url();
  // LOGN("[FileServer] Handling " + path);
  if (request->hasParam("delete")) {
    if (FileFS.exists(path)) {
      LOGF("[FileServer] Delete %s\n", path);
      FileFS.remove(path);
      request->redirect("/");
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
    LOGF("[FileServer] Sending %s\n", path);

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