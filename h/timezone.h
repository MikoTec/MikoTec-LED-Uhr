const char timezone_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<title>Zeitzone</title>
<meta name=viewport content="width=device-width, initial-scale=1">
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
$externallinks
<style>
.tz-tabs {
  display: flex;
  border-bottom: 2px solid #e0e0e0;
  margin-bottom: 20px;
}
.tz-tab {
  padding: 10px 22px;
  cursor: pointer;
  font-family: Abel, sans-serif;
  font-size: 14px;
  letter-spacing: 1px;
  text-transform: uppercase;
  color: #888;
  border-bottom: 2px solid transparent;
  margin-bottom: -2px;
  transition: color 0.15s, border-color 0.15s;
  background: none;
  border-top: none;
  border-left: none;
  border-right: none;
  outline: none;
}
.tz-tab.active {
  color: #333;
  border-bottom: 2px solid #333;
  font-weight: bold;
}
.tz-pane { display: none; }
.tz-pane.active { display: block; }
.tz-input {
  width: 100%;
  box-sizing: border-box;
  padding: 8px 10px;
  font-size: 15px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fafafa;
  margin: 4px 0 12px 0;
  font-family: Abel, sans-serif;
}
.tz-search-row {
  display: flex;
  gap: 8px;
  align-items: center;
  margin-bottom: 12px;
}
.tz-search-row input {
  flex: 1;
  padding: 8px 10px;
  font-size: 15px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: #fafafa;
  font-family: Abel, sans-serif;
  box-sizing: border-box;
}
#searchStatus {
  font-size: 12px;
  color: #888;
  margin-top: 4px;
  display: block;
}
select.tz-input {
  appearance: auto;
}
</style>
</head>
<body class=settings-page>
$menu

<div id="rcorners2" style="max-width:480px;margin:20px auto;">
  <label class="section-head">Zeitzone einstellen</label>

  <div class="tz-tabs">
    <button class="tz-tab active" onclick="showTab('GPS',this)">Suche</button>
    <button class="tz-tab" onclick="showTab('Manual',this)">Manuell</button>
    <button class="tz-tab" onclick="showTab('City',this)">Stadt</button>
  </div>

  <!-- Tab: Suche -->
  <div id="GPS" class="tz-pane active">
    <form action=/ method=GET>
    <div class="tz-search-row">
      <input type=text id=searchAddr placeholder="Adresse oder Stadt eingeben...">
      <button type=button class="btn btn-sm" onclick="searchLocation()">Suchen</button>
    </div>
    <span id=searchStatus></span>
    <label>Breitengrad</label>
    <input class="tz-input" type=text name=latitude id=latitude value=$latitude>
    <label>Laengengrad</label>
    <input class="tz-input" type=text name=longitude id=longitude value=$longitude>
    <div style="text-align:center;margin-top:10px;">
      <input class="btn btn-default" type=submit name=submit value='Update Timezone'>
    </div>
    </form>
  </div>

  <!-- Tab: Manuell -->
  <div id="Manual" class="tz-pane">
    <form action=/ method=GET>
    <label>UTC Versatz (z.B. 1 fuer MEZ, 2 fuer MESZ)</label>
    <input class="tz-input" type=text name=timezone id=timezone value=$timezone>
    <div style="text-align:center;margin-top:10px;">
      <input class="btn btn-default" type=submit name=submit value='Update Timezone'>
    </div>
    </form>
  </div>

  <!-- Tab: Stadt -->
  <div id="City" class="tz-pane">
    <form action=/ method=GET>
    <label>Stadt auswaehlen</label>
    <select class="tz-input" id=citysel onchange="cityChanged()">
      <option value="">-- Stadt waehlen --</option>
      <option value="52.52,13.41">Berlin</option>
      <option value="48.14,11.58">Muenchen</option>
      <option value="53.55,9.99">Hamburg</option>
      <option value="50.94,6.96">Koeln</option>
      <option value="50.11,8.68">Frankfurt</option>
      <option value="48.78,9.18">Stuttgart</option>
      <option value="51.23,6.78">Duesseldorf</option>
      <option value="51.51,7.47">Dortmund</option>
      <option value="51.45,7.01">Essen</option>
      <option value="51.17,7.08">Solingen</option>
      <option value="51.05,7.00">Wuppertal</option>
      <option value="51.34,12.37">Leipzig</option>
      <option value="51.05,13.74">Dresden</option>
      <option value="48.40,10.00">Ulm</option>
      <option value="49.45,11.08">Nuernberg</option>
      <option value="47.37,8.54">Zuerich</option>
      <option value="48.21,16.37">Wien</option>
      <option value="46.95,7.45">Bern</option>
      <option value="51.50,-0.13">London</option>
      <option value="48.86,2.35">Paris</option>
      <option value="40.42,-3.70">Madrid</option>
      <option value="41.90,12.50">Rom</option>
      <option value="52.37,4.90">Amsterdam</option>
      <option value="59.33,18.07">Stockholm</option>
      <option value="55.68,12.57">Kopenhagen</option>
      <option value="60.17,24.94">Helsinki</option>
      <option value="38.72,-9.14">Lissabon</option>
      <option value="50.08,14.44">Prag</option>
      <option value="47.50,19.04">Budapest</option>
      <option value="52.23,21.01">Warschau</option>
      <option value="40.71,-74.01">New York</option>
      <option value="34.05,-118.24">Los Angeles</option>
      <option value="35.69,139.69">Tokyo</option>
      <option value="-33.87,151.21">Sydney</option>
    </select>
    <input type=text name=latitude id=citylat style="display:none">
    <input type=text name=longitude id=citylng style="display:none">
    <div style="text-align:center;margin-top:10px;">
      <input class="btn btn-default" type=submit name=submit value='Update Timezone'>
    </div>
    </form>
  </div>

</div>

<script>
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
</script>
</body>
</html>
)=====" ;
