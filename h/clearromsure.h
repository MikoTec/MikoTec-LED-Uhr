const char clearromsure_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<link rel=stylesheet href=style.css>
</head>
<body class=settings-page>
$menu
<H1>Bist du sicher?</H1><br>
Alle Einstellungen inkl. WLAN-Passwort werden geloescht. Nach Neustart bitte neu verbinden.<br><br><br>
<a class="btn" href=/cleareeprom>Werksreset</a>
<br><br>
<a href=/settings>Zurueck zu Einstellungen</a>
</body>
</html>
)=====";
