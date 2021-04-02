function disableEnterSubmit() {
  window.addEventListener(
    "keydown",
    function (e) {
      if (
        e.keyIdentifier == "U+000A" ||
        e.keyIdentifier == "Enter" ||
        e.key == "Enter"
      ) {
        if (e.target.nodeName == "INPUT" && e.target.type == "text") {
          e.preventDefault();
          return false;
        }
      }
    },
    true
  );
}

function isUrl(s) {
  var regexp = /(ftp|http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/;
  return regexp.test(s);
}

window.addEventListener("DOMContentLoaded", function (e) {
  console.log("DOMContentLoaded");
  disableEnterSubmit();
  var s = window.localStorage;
  var os = s["server-url"] || serverUrl || window.location.origin || "";
  document.getElementById("current-server").textContent = os;
  document
    .getElementById("server-submit")
    .addEventListener("click", function (e) {
      let newUrl = document.getElementById("new-server").value || "";
      console.log(newUrl);
      if (!newUrl || isUrl(newUrl)) {
        s["server-url"] = newUrl;
        location.reload(true);
      } else {
        alert("不合法的服务器地址！");
      }
    });

  document.getElementById("back-home").addEventListener("click", function (e) {
    // window.location.replace(location.replace(location.split("/").pop(), ""));
    location.href = location.href.replace(/[^/]*$/, '');
  });
});
