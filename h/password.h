const char password_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<link rel=stylesheet href='clockmenustyle.css'>
</head>
<body class=settings-page>

<a href='/'>Netzwerk</a>-> <strong>Passwort</strong> -> Timezone<BR>
<h1>Passwort</h1>
<form class=form-verticle action=/timezonesetup method=GET>
<ul>
<label>Passwort: <input type=password name=pass id="pass"/></label><br>
<input type=submit value=Weiter>
</form>
</body>
<script>function otherssid(){a=document.getElementById("other_ssid");a.checked=true}function regularssid(){a=document.getElementById("other_text");a.value=""};</script>
</html>
)=====";
