function loadData(e) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status < 400) {
      var files = JSON.parse(xhr.responseText);
      console.log(files);
      var lines = files.map(function (it) {
        let name = it["n"];
        let size = it["z"];
        var ca = $("<a>")
          .attr("href", serverUrl + name)
          .attr("target", "_blank")
          .text(name);
        let caz = $("<span>").text(" (" + size + " bytes)");
        var aa = $("<button>").text("删除");
        aa.on("click", function (e) {
          var cf = confirm("确定删除这个文件：[" + name + "] ?");
          if (!cf) {
            return false;
          }
          var ar = new XMLHttpRequest();
          ar.onload = function () {
            console.log(ar.statusText);
            loadData();
          };
          var fd = new FormData();
          fd.append("file_path", name);
          ar.open("POST", serverUrl + "/api/delete_file");
          ar.send(fd);
        });
        var da = $("<button>").text("Download");
        da.on("click", function (e) {
          var link = document.createElement("a");
          link.download = name.substring(it.lastIndexOf("/") + 1);
          link.href = serverUrl + name;
          document.body.appendChild(link);
          link.click();
          link.remove();
          console.log(link.outerHTML);
        });
        var ea = $("<button>").text("查看");
        ea.on("click", function (e) {
          var url =
            "edit.html?" +
            encodeURIComponent("url") +
            "=" +
            encodeURIComponent(name);
          window.open(url, "_blank");
        });
        return $("<p>").append(ca, caz, aa, ea);
      });
      $("#output").html("");
      lines.forEach((it) => $("#output").append(it));
      // $('#output').append(lines.map((it) => it.html()));
    } else {
      console.error("Failed: " + xhr.statusText);
      $("output").html("<p>Error: " + xhr.statusText + "</p>");
    }
  };
  xhr.onerror = function (e) {
    console.error("Failed: " + e);
  };
  xhr.open("GET", serverUrl + "/api/files");
  xhr.send();
}

window.addEventListener("DOMContentLoaded", function (e) {
  serverUrl = serverUrl || window.localStorage["server-url"] || "";
  console.log("DOMContentLoaded");
  console.log(serverUrl);
  loadData(e);
  $("#reload").on("click", function (e) {
    console.log("click");
    loadData(e);
    return false;
  });
});
