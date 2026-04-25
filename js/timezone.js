// Zeitzone JavaScript
function showTab(id, btn) {
  var panes = document.getElementsByClassName("tz-pane");
  for (var i = 0; i < panes.length; i++) panes[i].className = "tz-pane";
  document.getElementById(id).className = "tz-pane active";
  var tabs = document.getElementsByClassName("tz-tab");
  for (var i = 0; i < tabs.length; i++) tabs[i].className = "tz-tab";
  btn.className = "tz-tab active";
}
function searchLocation() {
  var addr = document.getElementById("searchAddr").value;
  if (!addr) { document.getElementById("searchStatus").innerText = "Bitte Adresse eingeben"; return; }
  document.getElementById("searchStatus").innerText = "Suche...";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "https://nominatim.openstreetmap.org/search?q=" + encodeURIComponent(addr) + "&format=json&limit=1", true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      if (xhr.status == 200) {
        var res = JSON.parse(xhr.responseText);
        if (res.length > 0) {
          document.getElementById("latitude").value = Math.round(parseFloat(res[0].lat) * 100) / 100;
          document.getElementById("longitude").value = Math.round(parseFloat(res[0].lon) * 100) / 100;
          document.getElementById("searchStatus").innerText = "Gefunden: " + res[0].display_name.substring(0, 60) + "...";
        } else { document.getElementById("searchStatus").innerText = "Nichts gefunden."; }
      } else { document.getElementById("searchStatus").innerText = "Fehler bei der Suche."; }
    }
  };
  xhr.send();
}
function cityChanged() {
  var s = document.getElementById("citysel").value;
  if (s != "") {
    var p = s.split(",");
    document.getElementById("citylat").value = p[0];
    document.getElementById("citylng").value = p[1];
    document.getElementById("latitude").value = p[0];
    document.getElementById("longitude").value = p[1];
  }
}
