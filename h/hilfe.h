const char hilfe_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<title>Hilfe</title>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
$externallinks
</head>
<body class=settings-page>

<div id="rcorners2">
<label class="section-head">Hauptseite</label>
<form class=form-verticle>
<ul>
<li>
<label data-tooltip="Waehle die Farben fuer Stunden- und Minutenzeiger ueber den Farbwaehler." class="tooltip" style="cursor:help">Stundenfarbe / Minutenfarbe &#9432;</label>
</li>
<li>
<label data-tooltip="Bestimmt wie stark die Farben ineinander uebergehen. 0 = kein Mischen, 255 = voller Uebergang." class="tooltip" style="cursor:help">Mischpunkt &#9432;</label>
</li>
<li>
<label data-tooltip="Gesamthelligkeit aller LEDs. Wird durch den Uhrentyp begrenzt (Netzstrom=255, USB=100)." class="tooltip" style="cursor:help">Helligkeit &#9432;</label>
</li>
<li>
<label data-tooltip="Speichere deine aktuelle Farbkombination in einem von drei Speicherplaetzen. Lade sie jederzeit wieder." class="tooltip" style="cursor:help">Speichern / Laden V1-V3 &#9432;</label>
</li>
<li>
<label data-tooltip="Uebertraegt die aktuellen Farb- und Helligkeitswerte auf die Uhr." class="tooltip" style="cursor:help">Update &#9432;</label>
</li>
</ul>
</form>
</div>

<div id="rcorners2">
<label class="section-head">Einstellungen</label>
<form class=form-verticle>
<ul>
<li>
<label data-tooltip="Zeigt die aktuelle Uhrzeit des ESP. Per Klick auf die Zeit startet das Spiel. Daneben kann die Zeit manuell gesetzt werden." class="tooltip" style="cursor:help">Uhrzeit &#9432;</label>
</li>
<li>
<label data-tooltip="Markierungen auf dem Zifferblatt: Keine / Mittag / Quadranten (3,6,9,12) / Stunden (alle 12) / Abdunkeln (zum Mittag hin dunkler)." class="tooltip" style="cursor:help">Std-Marken &#9432;</label>
</li>
<li>
<label data-tooltip="Zeigt den Sekundenzeiger als invertierte (umgekehrte Farbe) LED an." class="tooltip" style="cursor:help">Sekunden &#9432;</label>
</li>
<li>
<label data-tooltip="Simuliert eine Stunde vor dem Aufwachen einen langsamen Sonnenaufgang auf den LEDs." class="tooltip" style="cursor:help">Sonnenaufgang &#9432;</label>
</li>
</ul>
</form>
</div>

<div id="rcorners2">
<label class="section-head">Schlafmodus</label>
<form class=form-verticle>
<ul>
<li>
<label data-tooltip="Schwarz: Alle LEDs aus. Punkte: Nur Stunden-/Minutenpunkt sichtbar. Gedimmt: Normales Zifferblatt sehr dunkel. Mondphase: Aktuelle Mondphase. Aus: Schlafmodus deaktiviert." class="tooltip" style="cursor:help">Schlaf-Typ &#9432;</label>
</li>
<li>
<label data-tooltip="Manuell: Feste Schlaf-/Wachzeiten. Automatisch: Berechnet Sonnenauf- und untergang anhand der gespeicherten Koordinaten." class="tooltip" style="cursor:help">Schlafsteuerung &#9432;</label>
</li>
<li>
<label data-tooltip="Feste Uhrzeiten fuer den Beginn und das Ende des Schlafmodus. Nur bei manueller Schlafsteuerung aktiv." class="tooltip" style="cursor:help">Von / Bis &#9432;</label>
</li>
<li>
<label data-tooltip="Beeinflusst die Darstellung der Mondphase. Nordhalbkugel: zunehmender Mond rechts. Suedhalbkugel: zunehmender Mond links." class="tooltip" style="cursor:help">Hemispaere &#9432;</label>
</li>
</ul>
</form>
</div>

<div id="rcorners2">
<label class="section-head">Zeitzone</label>
<form class=form-verticle>
<ul>
<li>
<label data-tooltip="Ermittelt den Standort per Browser-GPS und setzt Breitengrad/Laengengrad automatisch." class="tooltip" style="cursor:help">GPS &#9432;</label>
</li>
<li>
<label data-tooltip="Waehle eine Stadt aus der Liste. Koordinaten und Zeitzone werden automatisch gesetzt." class="tooltip" style="cursor:help">Stadt &#9432;</label>
</li>
<li>
<label data-tooltip="UTC-Versatz direkt als Zahl eingeben (z.B. 1 fuer MEZ, 2 fuer MESZ)." class="tooltip" style="cursor:help">Manuell &#9432;</label>
</li>
<li>
<label data-tooltip="Sommerzeit manuell aktivieren oder deaktivieren. Addiert eine Stunde zur Zeitzone." class="tooltip" style="cursor:help">Sommerzeit &#9432;</label>
</li>
</ul>
</form>
</div>

<div id="rcorners2">
<label class="section-head">Uhrentyp &amp; Sonstiges</label>
<form class=form-verticle>
<ul>
<li>
<label data-tooltip="Original: 120 LEDs, Netzstrom (max. 255). Mini: 60 LEDs, USB (max. 100). Benutzerdefiniert: Eigene LED-Anzahl und Stromversorgung." class="tooltip" style="cursor:help">Uhrentyp &#9432;</label>
</li>
<li>
<label data-tooltip="Name der Uhr im Netzwerk (mDNS). Erreichbar unter http://[name].local im Browser." class="tooltip" style="cursor:help">Uhrenname &#9432;</label>
</li>
<li>
<label data-tooltip="Setzt einen Timer. Die LEDs fuellen sich in der gewaehlten Alarmfarbe und blinken am Ende." class="tooltip" style="cursor:help">Alarm &#9432;</label>
</li>
<li>
<label data-tooltip="Neue Firmware (.bin Datei) per Browser hochladen (OTA Update). Erreichbar unter /update." class="tooltip" style="cursor:help">Firmware Update &#9432;</label>
</li>
<li>
<label data-tooltip="Loescht alle Einstellungen inkl. WLAN-Daten. Die Uhr startet im Access-Point-Modus zur Neukonfiguration." class="tooltip" style="cursor:help">Werksreset &#9432;</label>
</li>
</ul>
</form>
</div>

<div class=btn-box>
<a class="btn btn-default" href=/>Hauptseite</a>
<a class="btn btn-default" href=/settings>Einstellungen</a>
<a class="btn btn-default" href=/timezone>Zeitzone</a>
<a class="btn btn-default" href=/support>Support</a>
</div>

</body>
</html>
)=====";
