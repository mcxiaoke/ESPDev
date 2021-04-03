var serverVersion = "";
const TIME_OUT_MS = 5000;

function newLine() {
  return document.createElement("p");
}

function buildOutputDiv(d) {
  let now = Date.now();
  let task = d["task"];
  var lastRunAt = "";
  if (d["last_start"] < 1) {
    lastRunAt = "N/A";
  } else {
    let lastStart = now - (d["up_time"] - d["last_start"]);
    lastRunAt = moment(lastStart).format("MM-DD HH:mm:ss");
  }

  var nextRunAt = "";
  var nextRemains = 0;
  let timeStart = task["start"];
  if (task["prev"] > 0) {
    nextRemains = task["prev"] + task["interval"] - task["up_time"];
  } else {
    nextRemains = timeStart + task["interval"] - task["up_time"];
  }
  if (timeStart < 1) {
    nextRunAt = "N/A";
  } else {
    nextRunAt = moment(now + nextRemains * 1000).format("MM-DD HH:mm:ss");
  }
  let surl = serverUrl || window.location.href;
  let debugMode = d["debug"] == 1 ? "开发版" : "正式版";
  let dbClass = d["debug"] == 1 ? "danger" : "important";
  let tb = $("<table>").append(
    $("<tr>")
      .attr("class", "p-head")
      .append($("<td>").text("远程服务: "), $("<td>").text(surl)),
    $("<tr>")
      .attr("class", "p-head")
      .append(
        $("<td>").text("开机时刻: "),
        $("<td>").text(moment.unix(d["boot_time"]).format("YYYY-MM-DD HH:mm:ss"))),
    $("<tr>")
      .attr("class", "p-head")
      .append($("<td>").text("系统版本: "), $("<td>").text(d["revision"])),
    $("<tr>")
      .attr("class", "p-sketch")
      .append($("<td>").text("固件指纹: "), $("<td>").text(d["sketch"])),

    $("<tr>")
      .attr("class", "p-head")
      .append(
        $("<td>").text("每次持续时间: "),
        $("<td>").addClass("important").text(d["duration"] + "秒")
      ),
    $("<tr>")
      .attr("class", "p-status")
      .append(
        $("<td>").text("当前系统状态: "),
        $("<td>")
          .addClass("important")
          .text(d["on"] ? "正在浇水" : "空闲"),
      ),
  );

  let o = $("<div>").attr("id", "output").attr("class", "output");
  return o.append(tb, $("<p>"));
}

function buildFormDiv(d) {
  let btnPump = document.createElement("button");
  btnPump.textContent = d["on"] ? "停止浇水" : "开始浇水";
  btnPump.setAttribute("id", "btn-pump");
  btnPump.setAttribute("class", "important button-large");
  btnPump.onclick = function (e) {
    let checkStart = new Date().getTime();
    let checkId = setInterval(function () {
      loadData(false);
      if (new Date().getTime() - checkStart > 15000) {
        clearInterval(checkId);
      }
    }, 1000);
    xhr = new XMLHttpRequest();
    xhr.onload = function () {
      console.log("pump button.");
      loadData(false);
    };
    xhr.open(
      "POST",
      serverUrl + "/api/control?token=pump&args=" + (d["on"] ? "stop" : "start")
    );
    xhr.send();
    return true;
  };

  // let btnSwitch = document.createElement("button");
  // btnSwitch.textContent = d["enabled"] ? "禁用定时" : "启用定时";
  // btnSwitch.setAttribute("id", "btn-switch");
  // btnSwitch.setAttribute("class", "important button-large");
  // btnSwitch.onclick = function (e) {
  //   xhr = new XMLHttpRequest();
  //   xhr.onload = function () {
  //     console.log("switch button.");
  //     loadData(false);
  //   };
  //   xhr.open(
  //     "POST",
  //     serverUrl +
  //     "/api/control?token=pump&args=" +
  //     (d["enabled"] ? "off" : "on")
  //   );
  //   xhr.send();
  //   return true;
  // };

  // let btnTimer = document.createElement("button");
  // btnTimer.textContent = "重置定时";
  // btnTimer.setAttribute("id", "btn-timer");
  // btnTimer.setAttribute("class", "danger button-large");
  // btnTimer.onclick = function (e) {
  //   // e.preventDefault();
  //   let cf = confirm("确定重置定时器吗?");
  //   if (cf) {
  //     xhr = new XMLHttpRequest();
  //     xhr.onload = function () {
  //       console.log("timer reset.");
  //       // location.reload(true);
  //       loadData(false);
  //     };
  //     xhr.open("POST", serverUrl + "/api/control?token=pump&args=reset");
  //     xhr.send();
  //     return true;
  //   }
  //   return false;
  // };

  let btnReboot = document.createElement("button");
  btnReboot.textContent = "重启机器";
  btnReboot.setAttribute("id", "btn-reboot");
  btnReboot.setAttribute("class", "danger button-large");
  btnReboot.onclick = function (e) {
    var cf = confirm("确定要重启浇水器设备吗?");
    if (cf) {
      e.preventDefault();
      xhr = new XMLHttpRequest();
      xhr.onload = function () {
        if (xhr.status < 400) {
          alert("Board will reboot!");
          setTimeout(() => {
            location.reload(true);
          }, 15000);
        }
      };
      xhr.open("POST", serverUrl + "/api/control?token=pump&args=reboot");
      xhr.send();
      return true;
    }
    return false;
  };

  var buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "form-div");
  buttonDiv.append(
    btnPump,
    btnReboot,
    newLine(),
    // btnTimer,
    // btnSwitch,
    // newLine()
  );
  return buttonDiv;
}

function buildTitleDiv(debugMode = false) {
  let div = document.createElement("div");
  div.setAttribute("id", "title-div");
  let t = document.createElement("h1");
  if (debugMode) {
    t.setAttribute("class", "danger");
    t.textContent = "智能浇水器（开发版）";
  } else {
    t.textContent = "智能浇水器";
  }
  div.append(t);
  return div;
}

function buildButtonDiv() {
  let buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "button-div-2");

  let btnRaw = document.createElement("button");
  btnRaw.textContent = "系统日志";
  btnRaw.setAttribute("id", "btn-raw");
  btnRaw.setAttribute("class", "button-large");
  btnRaw.onclick = (e) => {
    window.open(
      serverUrl + "/serial",
      "_blank"
    );
  };

  let btnFiles = document.createElement("button");
  btnFiles.textContent = "查看文件";
  btnFiles.setAttribute("id", "btn-files");
  btnFiles.setAttribute("class", "button-large");
  // btnFiles.onclick = (e) => (window.location.href = "files.html");
  btnFiles.onclick = (e) => {
    window.open("files.html",
      "_blank"
    );
  };

  let btnOption = document.createElement("button");
  btnOption.textContent = "配置修改";
  btnOption.setAttribute("id", "btn-option");
  btnOption.setAttribute("class", "button-large");
  btnOption.onclick = (e) => (window.location.href = "option.html");

  let btnOTA = document.createElement("button");
  btnOTA.textContent = "系统更新";
  btnOTA.setAttribute("id", "btn-ota");
  btnOTA.setAttribute("class", "button-large");
  btnOTA.onclick = (e) => (window.location.href = "update.html");

  buttonDiv.append(btnRaw, btnFiles, newLine(), btnOption, btnOTA, newLine());

  return buttonDiv;
}

function buildButtonDiv2() {
  let buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "button-div");

  let btnClear = document.createElement("button");
  btnClear.textContent = "清空日志";
  btnClear.setAttribute("id", "btn-clear");
  btnClear.setAttribute("class", "danger button-large");
  btnClear.onclick = function (e) {
    e.preventDefault();
    let cf = confirm("确定清空所有日志文件吗?");
    if (cf) {
      xhr = new XMLHttpRequest();
      xhr.onload = function () {
        if (xhr.status < 400) {
          // alert("Server logs is cleared!");
          console.log("Clear logs ok.");
          // location.reload(true);
        }
      };
      xhr.open("POST", serverUrl + "/api/control?token=pump&args=clear");
      xhr.send();
      return true;
    }
    return false;
  };

  return buttonDiv;
}

function handleError(firstTime) {
  if (firstTime) {
    let outputDiv = $("<div>").attr("id", "error").attr("class", "output");
    outputDiv.append(
      $("<p>")
        .addClass("danger")
        .text("服务器离线，无法连接!!!"),
      $("<p>")
        .addClass("danger")
        .text("请检查配置: " + serverUrl)
    );
    $("#content").append(outputDiv);
  }
}

function loadData(firstTime) {
  serverUrl = serverUrl || window.localStorage["server-url"] || "";
  console.log("loadData from ", serverUrl);
  let xhr = new XMLHttpRequest();
  xhr.timeout = TIME_OUT_MS;
  xhr.ontimeout = function (e) {
    console.log("ontimeout");
    handleError(firstTime);
  };
  xhr.onerror = function (e) {
    console.log("onerror");
    handleError(firstTime);
  };
  xhr.onloadend = function (e) {
    let output = document.createElement("div");
    output.innerHTML = document.getElementById("content").outerHTML;
    // console.log(output);
  };
  xhr.onload = function (e) {
    console.log("onload " + xhr.status);
    if (xhr.status < 400) {
      let d = JSON.parse(xhr.responseText);
      console.log("Data:", d);
      $("#content").html("");
      $("#content").append(
        buildTitleDiv(d["debug"] == 1),
        buildOutputDiv(d),
        buildFormDiv(d),
        buildButtonDiv()
      );

      if (serverVersion && d["version"] != serverVersion) {
        serverVersion = d["version"];
        console.log("serverVersion=", serverVersion);
        location.reload(true);
      }
    } else {
      handleError(firstTime);
    }
  };
  xhr.timeout = TIME_OUT_MS;
  xhr.open("GET", serverUrl + "/api/status");
  xhr.setRequestHeader("Accept", "application/json");
  xhr.send();
  return false;
}

function onReady(e) {
  $("#content").append(buildTitleDiv(false), buildButtonDiv());
  loadData(true);
  setInterval(function () {
    loadData(false);
  }, TIME_OUT_MS);
}


window.addEventListener("DOMContentLoaded", onReady);
