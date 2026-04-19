const char timezone_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head><title>Zeitzone</title>
<meta name=viewport content="width=device-width, initial-scale=1">
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<link rel=stylesheet href=http://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css>
<script src=https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js></script>
<script src=http://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/js/bootstrap.min.js></script>
<link rel=stylesheet href='clockmenustyle.css'>
</head>
<body class="settings-page">
<div class=container>
<h2>Zeitzone einstellen</h2>
<ul class="nav nav-tabs">
<li class=active><a data-toggle=tab href=#GPS>GPS</a></li>
<li><a data-toggle=tab href=#Manual>Manuell</a></li>
<li><a data-toggle=tab href=#City>Stadt</a></li>
</ul>
<div class=tab-content>
<div id=GPS class="tab-pane fade in active">
<h3>Standort</h3>
<button type=button onclick=getLocation()>Mein GPS abrufen</button><br>
<form action=/ method=GET>
Breitengrad:<input type=text name=latitude id=latitude value=$latitude><br>
Laengengrad:<input type=text name=longitude id=longitude value=$longitude><br>
<input type=submit name=submit value='Update Timezone'/></form>
</div>
<div id=Manual class="tab-pane fade">
<form action=/ method=GET>
UTC Versatz <input type=text name=timezone id=timezone value=$timezone><br>
<input type=submit name=submit value='Update Timezone'/></form>
</div>
<div id=City class="tab-pane fade">
<h3>Stadt waehlen</h3>
<form action=/ method=GET>
<select id=citysel onchange="cityChanged()" style="font-size:16px;padding:6px;width:100%;margin:8px 0">
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
<input type=submit name=submit value='Update Timezone'/></form>
</div>
</div>
</div>
<div class=btn-box style="margin-top:15px">
<a class=btn href=/>Zurueck</a>
<a class=btn href=/settings>Einstellungen</a>
<a class=btn href=/hilfe>Hilfe</a>
</div>
<script>var x=document.getElementById("latitude");var y=document.getElementById("longitude");function getLocation(){if(navigator.geolocation){navigator.geolocation.getCurrentPosition(showPosition)}else{alert("Geolocation wird von diesem Browser nicht unterstuetzt.")}}function showPosition(a){console.log("in showPosition");x.value=Math.round(a.coords.latitude*100)/100;y.value=Math.round(a.coords.longitude*100)/100}function cityChanged(){var s=document.getElementById("citysel").value;if(s!=""){var p=s.split(",");document.getElementById("citylat").value=p[0];document.getElementById("citylng").value=p[1];document.getElementById("latitude").value=p[0];document.getElementById("longitude").value=p[1];}}</script>
</body>
</html>
)=====";
