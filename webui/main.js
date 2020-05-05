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
  let tb = $("<table>").append(
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
        $("<td>").text("Pump Enabled: "),
        $("<td>").text(d["enabled"] ? "True" : "False"),
        $("<td>").text("Free Heap: "),
        $("<td>").text(d["heap"])
      )
  );

  let o = $("<div>").attr("id", "output").attr("class", "output");
  return o.append(tb, $("<p>"));
}

function buildFormDiv(d) {
  let btnPump = document.createElement("button");
  btnPump.textContent = d["on"] ? "Stop Pump" : "Start Pump";
  btnPump.setAttribute("id", "btn-pump");
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
  btnSwitch.textContent = d["enabled"] ? "Disable Pump" : "Enable Pump";
  btnSwitch.setAttribute("id", "btn-switch");
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

  var divider = document.createElement("p");
  var buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "form-div");
  buttonDiv.append(btnPump, btnSwitch, divider);
  return buttonDiv;
}

function buildButtonDiv() {
  let buttonDiv = document.createElement("div");
  buttonDiv.setAttribute("id", "button-div");

  let btnRaw = document.createElement("button");
  btnRaw.textContent = "View Logs";
  btnRaw.setAttribute("id", "btn-raw");
  btnRaw.onclick = (e) => {
    window.open(
      serverUrl + "/logs/log-" + moment().format("YYYY-MM") + ".txt",
      "_blank"
    );
  };

  let btnClear = document.createElement("button");
  btnClear.textContent = "Clear Logs";
  btnClear.setAttribute("id", "btn-clear");
  // btnClear.setAttribute("type", "submit");
  btnClear.onclick = function (e) {
    e.preventDefault();
    let cf = confirm("Are you sure to delete all logs?");
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
  btnFiles.textContent = "Files";
  btnFiles.setAttribute("id", "btn-files");
  btnFiles.onclick = (e) => (window.location.href = "files.html");

  let btnOTA = document.createElement("button");
  btnOTA.textContent = "Update";
  btnOTA.setAttribute("id", "btn-ota");
  btnOTA.onclick = (e) => (window.location.href = "update.html");

  let btnTimer = document.createElement("button");
  btnTimer.textContent = "Reset";
  btnTimer.setAttribute("id", "btn-timer");
  btnTimer.onclick = function (e) {
    // e.preventDefault();
    let cf = confirm("Are you sure to reset timer?");
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

  let hr = document.createElement("p");
  buttonDiv.append(btnRaw, btnClear, btnFiles, hr, btnOTA, btnTimer, btnReboot);
  return buttonDiv;
}

function handleError(firstTime) {
  if (firstTime) {
    let outputDiv = $("<div>").attr("id", "output").attr("class", "output");
    outputDiv.append($("<p>").text("Failed to load data."));
    $("#content").append(outputDiv, buildButtonDiv());
  }
}

function loadData(firstTime) {
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
    console.log(output);
  };
  xhr.onload = function (e) {
    console.log("onload " + xhr.status);
    if (xhr.status < 400) {
      let d = JSON.parse(xhr.responseText);
      console.log(d);
      let c = document.getElementById("content");
      let t = document.createElement("h1");
      t.textContent = "Pump Dashboard";
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
