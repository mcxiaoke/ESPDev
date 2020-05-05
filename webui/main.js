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
  var tb = $("<table>").append(
    $("<tr>")
      .attr("id", "p-status")
      .append(
        $("<td>").text("Date: "),
        $("<td>").text(moment.unix(d["time"]).format("MM-DD HH:mm:ss")),
        $("<td>").text("Status: "),
        $("<td>").text(d["on"] ? "Running" : "Idle")
      ),
    $("<tr>")
      .attr("id", "p-last")
      .append(
        $("<td>").text("Last RunAt: "),
        $("<td>").text(lastRunAt),
        $("<td>").text("Elapsed: "),
        $("<td>").text(d["last_elapsed"] + "s")
      ),
    $("<tr>")
      .attr("id", "p-next")
      .append(
        $("<td>").text("Next RunAt: "),
        $("<td>").text(nextRunAt),
        $("<td>").text("Remains: "),
        $("<td>").text(humanElapsed(nextRemains))
      ),
    $("<tr>")
      .attr("id", "p-status")
      .append(
        $("<td>").text("Total Elapsed: "),
        $("<td>").text(d["total_elapsed"] + "s"),
        $("<td>").text("Up Time: "),
        $("<td>").text(humanElapsed(d["up_time"]))
      ),
    $("<tr>")
      .attr("id", "p-task")
      .append(
        $("<td>").text("Task Interval: "),
        $("<td>").text(humanElapsed(d["interval"])),
        $("<td>").text("Task Duration: "),
        $("<td>").text(humanElapsed(d["duration"]))
      ),
    $("<tr>")
      .attr("id", "p-global")
      .append(
        $("<td>").text("Global Switch: "),
        $("<td>").text(d["enabled"] ? "On" : "Off"),
        $("<td>").text("Debug: "),
        $("<td>").text(d["debug"] ? "True" : "False")
      ),
    $("<tr>")
      .attr("id", "p-sys")
      .append(
        $("<td>").text("Free Stack: "),
        $("<td>").text(d["stack"] || "N/A"),
        $("<td>").text("Free Heap: "),
        $("<td>").text(d["heap"])
      )
  );

  var o = $("<div>").attr("id", "output").attr("class", "output");
  return o.append(tb, $("<p>"));
}

function buildFormDiv(d) {
  var pf = $("<form>")
    .attr("id", "pump-form")
    .attr("method", "POST")
    .attr(
      "action",
      serverUrl + "/api/control?token=pump&args=" + (d["on"] ? "stop" : "start")
    )
    .append(
      $("<button>")
        .attr("id", "pump_submit")
        .attr("type", "submit")
        .text(d["on"] ? "Stop Pump" : "Start Pump")
    );

  var sf = $("<form>")
    .attr("id", "switch-form")
    .attr("method", "POST")
    .attr(
      "action",
      serverUrl +
        "/api/control?token=pump&args=" +
        (d["enabled"] ? "off" : "on")
    )
    .append(
      $("<button>")
        .attr("id", "switch_submit")
        .attr("type", "submit")
        .text(d["enabled"] ? "Switch Off" : "Switch On")
    );

  return $("<div>").attr("id", "form-div").append(pf, sf, $("<p>"));
}

function buildButtonDiv() {
  var buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "button-div");

  var btnLogs = document.createElement("button");
  btnLogs.textContent = "View Logs";
  btnLogs.setAttribute("id", "btn-logs");
  btnLogs.onclick = (e) => (window.location.href = "logs.html");

  var btnRaw = document.createElement("button");
  btnRaw.textContent = "Full Logs";
  btnRaw.setAttribute("id", "btn-raw");
  btnRaw.onclick = (e) => {
    window.open(
      serverUrl + "/logs/log-" + moment().format("YYYY-MM") + ".txt",
      "_blank"
    );
  };

  var btnClear = document.createElement("button");
  btnClear.textContent = "Clear Logs";
  btnClear.setAttribute("id", "btn-clear");
  // btnClear.setAttribute("type", "submit");
  btnClear.onclick = function (e) {
    e.preventDefault();
    var cf = confirm("Are you sure to delete all logs?");
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

  var btnFiles = document.createElement("button");
  btnFiles.textContent = "Files";
  btnFiles.setAttribute("id", "btn-files");
  btnFiles.onclick = (e) => (window.location.href = "files.html");

  var btnOTA = document.createElement("button");
  btnOTA.textContent = "Update";
  btnOTA.setAttribute("id", "btn-ota");
  btnOTA.onclick = (e) => (window.location.href = "update.html");

  var btnTimer = document.createElement("button");
  btnTimer.textContent = "Reset Timer";
  btnTimer.setAttribute("id", "btn-timer");
  btnTimer.onclick = function (e) {
    e.preventDefault();
    var cf = confirm("Are you sure to reset timer?");
    if (cf) {
      xhr = new XMLHttpRequest();
      xhr.onload = function () {
        console.log("timer reset.");
        // location.reload(true);
        // loadData(false);
      };
      xhr.onloadend = function () {
        console.log("timer reset end.");
      };
      xhr.open("POST", serverUrl + "/api/control?token=pump&args=reset");
      xhr.send();
      return true;
    }
    return false;
  };

  var btnReboot = document.createElement("button");
  btnReboot.textContent = "Reboot";
  btnReboot.setAttribute("id", "btn-reboot");
  btnReboot.onclick = function (e) {
    var cf = confirm("Are you sure to reboot board?");
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

  var hr = document.createElement("p");

  buttonDiv.append(
    btnLogs,
    btnRaw,
    btnClear,
    hr,
    btnFiles,
    btnOTA,
    btnTimer,
    btnReboot
  );
  return buttonDiv;
}

function handleError(firstTime) {
  if (firstTime) {
    var outputDiv = $("<div>").attr("id", "output").attr("class", "output");
    outputDiv.append($("<p>").text("Failed to load data."));
    $("#content").append(outputDiv, buildButtonDiv());
  }
}

function loadData(firstTime) {
  var xhr = new XMLHttpRequest();
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
    var output = document.createElement("div");
    output.innerHTML = document.getElementById("content").outerHTML;
    console.log(output);
  };
  xhr.onload = function (e) {
    console.log("onload " + xhr.status);
    if (xhr.status < 400) {
      var d = JSON.parse(xhr.responseText);
      console.log(d);
      var c = document.getElementById("content");
      var t = document.createElement("h1");
      t.textContent = "Pump Home";
      // c.append(t, buildOutputDiv(d), buildFormDiv(d), buildButtonDiv(d));
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
