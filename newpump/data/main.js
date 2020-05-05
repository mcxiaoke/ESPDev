function buildOutputDiv(d) {
  let now = Date.now();
  var lastRunAt = "";
  if (d["last_start"] < 1) {
    lastRunAt = "N/A";
  } else {
    let lastStart = now - (d["up_time"] - d["last_start"]);
    lastRunAt = moment(lastStart).format("MM-DD HH:mm:ss");
  }

  var nextRunAt = "";
  let timeStart = Math.max(d["last_reset"], d["last_start"]);
  let nextRemains = timeStart + d["interval"] - d["up_time"];
  let nextStart = now + nextRemains * 1000;
  if (timeStart < 1) {
    nextRunAt = "N/A";
  } else {
    nextRunAt = moment(nextStart).format("MM-DD HH:mm:ss");
  }
  let surl = serverUrl || window.location.origin;
  let tb = $("<table>").append(
    $("<tr>")
      .attr("id", "p-head")
      .append($("<td>").text("服务器地址: "), $("<td>").text(surl)),
    $("<tr>")
      .attr("id", "p-status")
      .append(
        $("<td>").addClass("important").text("设备状态: "),
        $("<td>")
          .addClass("danger")
          .text(d["enabled"] ? "已启用" : "已禁用"),
        $("<td>").addClass("important").text("运行状态: "),
        $("<td>")
          .addClass("danger")
          .text(d["on"] ? "正在浇水" : "空闲")
      ),
    $("<tr>")
      .attr("id", "p-global")
      .append(
        $("<td>").text("开机时间: "),
        $("<td>").text(humanElapsed(d["up_time"])),
        $("<td>").text("空闲内存: "),
        $("<td>").text(d["heap"])
      ),

    $("<tr>")
      .attr("id", "p-task")
      .append(
        $("<td>").text("浇水间隔: "),
        $("<td>").addClass("important").text(humanElapsed(d["interval"])),
        $("<td>").text("浇水时长: "),
        $("<td>").addClass("important").text(humanElapsed(d["duration"]))
      ),
    $("<tr>")
      .attr("id", "p-last")
      .append(
        $("<td>").text("上次浇水时刻: "),
        $("<td>").text(lastRunAt),
        $("<td>").text("上次浇水时长: "),
        $("<td>").text(d["last_elapsed"] + "s")
      ),
    $("<tr>")
      .attr("id", "p-next")
      .append(
        $("<td>").text("下次浇水时刻: "),
        $("<td>").addClass("important").text(nextRunAt),
        $("<td>").text("总共浇水时长: "),
        $("<td>").text(d["total_elapsed"] + "s")
      ),
    $("<tr>")
      .attr("id", "p-time")
      .append(
        $("<td>").text("当前时间: "),
        $("<td>").text(moment.unix(d["time"]).format("MM-DD HH:mm:ss")),
        $("<td>").text("距离下次运行: "),
        $("<td>").addClass("important").text(humanElapsed(nextRemains))
      )
  );

  let o = $("<div>").attr("id", "output").attr("class", "output");
  return o.append(tb, $("<p>"));
}

function buildFormDiv(d) {
  let btnPump = document.createElement("button");
  btnPump.textContent = d["on"] ? "停止浇水" : "开始浇水";
  btnPump.setAttribute("id", "btn-pump");
  btnPump.setAttribute("class", "important");
  btnPump.onclick = function (e) {
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

  let btnSwitch = document.createElement("button");
  btnSwitch.textContent = d["enabled"] ? "禁用设备" : "启用设备";
  btnSwitch.setAttribute("id", "btn-switch");
  btnSwitch.setAttribute("class", "important");
  btnSwitch.onclick = function (e) {
    xhr = new XMLHttpRequest();
    xhr.onload = function () {
      console.log("switch button.");
      loadData(false);
    };
    xhr.open(
      "POST",
      serverUrl +
        "/api/control?token=pump&args=" +
        (d["enabled"] ? "off" : "on")
    );
    xhr.send();
    return true;
  };

  let btnTimer = document.createElement("button");
  btnTimer.textContent = "重置定时";
  btnTimer.setAttribute("id", "btn-timer");
  btnTimer.setAttribute("class", "danger");
  btnTimer.onclick = function (e) {
    // e.preventDefault();
    let cf = confirm("确定重置定时器吗?");
    if (cf) {
      xhr = new XMLHttpRequest();
      xhr.onload = function () {
        console.log("timer reset.");
        // location.reload(true);
        loadData(false);
      };
      xhr.open("POST", serverUrl + "/api/control?token=pump&args=reset");
      xhr.send();
      return true;
    }
    return false;
  };

  let btnReboot = document.createElement("button");
  btnReboot.textContent = "重启机器";
  btnReboot.setAttribute("id", "btn-reboot");
  btnReboot.setAttribute("class", "danger");
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

  var divider = document.createElement("p");
  var buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "form-div");
  buttonDiv.append(btnPump, btnSwitch, btnTimer, btnReboot, divider);
  return buttonDiv;
}

function buildButtonDiv() {
  let buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "button-div");

  let btnRaw = document.createElement("button");
  btnRaw.textContent = "查看日志";
  btnRaw.setAttribute("id", "btn-raw");
  btnRaw.onclick = (e) => {
    window.open(
      serverUrl + "/logs/log-" + moment().format("YYYY-MM") + ".txt",
      "_blank"
    );
  };

  let btnClear = document.createElement("button");
  btnClear.textContent = "清空日志";
  btnClear.setAttribute("id", "btn-clear");
  // btnClear.setAttribute("type", "submit");
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

  let btnFiles = document.createElement("button");
  btnFiles.textContent = "查看文件";
  btnFiles.setAttribute("id", "btn-files");
  btnFiles.onclick = (e) => (window.location.href = "files.html");

  let btnOption = document.createElement("button");
  btnOption.textContent = "配置修改";
  btnOption.setAttribute("id", "btn-option");
  btnOption.onclick = (e) => (window.location.href = "option.html");

  let btnOTA = document.createElement("button");
  btnOTA.textContent = "系统更新";
  btnOTA.setAttribute("id", "btn-ota");
  btnOTA.onclick = (e) => (window.location.href = "update.html");

  let hr = document.createElement("p");
  buttonDiv.append(btnRaw, btnFiles, btnOption, btnOTA);
  return buttonDiv;
}

function handleError(firstTime) {
  if (firstTime) {
    let outputDiv = $("<div>").attr("id", "output").attr("class", "output");
    outputDiv.append(
      $("<p>")
        .addClass("danger")
        .text("请检查服务器配置: " + serverUrl)
    );
    $("#content").append(outputDiv, buildButtonDiv());
  }
}

function loadData(firstTime) {
  serverUrl = serverUrl || window.localStorage["server-url"] || "";
  let xhr = new XMLHttpRequest();
  xhr.timeout = 3000;
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
      let c = document.getElementById("content");
      let t = document.createElement("h1");
      t.setAttribute("id", "pump-title");
      t.textContent = "智能浇水器";
      $("#content").html("");
      $("#content").append(
        t,
        buildOutputDiv(d),
        buildFormDiv(d),
        buildButtonDiv()
      );
    } else {
      handleError(firstTime);
    }
  };
  xhr.open("GET", serverUrl + "/api/status");
  xhr.setRequestHeader("Accept", "application/json");
  xhr.send();
  return false;
}

function onReady(e) {
  loadData(true);
  setInterval(function () {
    loadData(false);
  }, 10000);
}
window.addEventListener("DOMContentLoaded", onReady);
