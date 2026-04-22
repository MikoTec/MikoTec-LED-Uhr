# MikoTec LED Uhr

Eine ESP8266-basierte LED-Uhr mit NeoPixel-Ring, Webinterface und automatischen Updates.

## Features

- **LED-Uhranzeige** mit einstellbaren Stunden- und Minutenfarben, Überblendeffekt und Helligkeit
- **Mondphase** als Nachtanzeige – astronomisch korrekte Berechnung
- **Sonnenaufgang-Simulation** – Uhr simuliert eine Stunde vor Sonnenaufgang einen langsamen Dämmerungseffekt
- **Auto-Schlaf** – schaltet automatisch bei Sonnenuntergang ab und bei Sonnenaufgang wieder ein (berechnet anhand von Breitengrad/Längengrad)
- **Nacht-Helligkeit** – separate Helligkeitseinstellung für den Schlafmodus
- **4 Farbschemata** – speichere und lade verschiedene Farbkombinationen
- **Webinterface** – alle Einstellungen über den Browser konfigurierbar
- **OTA-Updates** – Firmware kann über den Browser oder automatisch aktualisiert werden
- **Automatische Updates** – Uhr prüft täglich einen Update-Server und installiert neue Versionen selbstständig
- **NTP-Zeitsynchronisation** – automatische Uhrzeit über das Internet
- **Sommerzeit** – automatische DST-Erkennung über TimezoneDB API
- **Standortsuche** – Koordinaten per OpenStreetMap Nominatim oder Städteauswahl

## Hardware

- **ESP8266** (NodeMCU v2)
- **NeoPixel WS2812B LED-Ring** (60 LEDs Standard)
- Anschluss über GPIO2 (D4) via UART1

## Projektstruktur

```
MikoTec-LED-Uhr/
├── MikoTec-LED-Uhr.ino    # Hauptsketch
├── h/                      # Header-Dateien (HTML/CSS/JS für Webinterface)
│   ├── root.h              # Startseite
│   ├── settings.h          # Einstellungen
│   ├── timezone.h          # Zeitzone (Suche/Manuell/Stadt)
│   ├── clockjs.h           # Uhr-Animation im Browser
│   ├── css.h               # Stylesheet
│   ├── alarm.h             # Alarm-Seite
│   ├── hilfe.h             # Hilfe-Seite
│   ├── support.h           # Support/Log-Seite
│   └── ...
├── lib/                    # Eingebettete Libraries
│   ├── NeoPixelBus/
│   ├── Time/
│   ├── NTP/
│   └── arduinoWebSockets-master/
├── updates/                # Firmware-Binaries für OTA
│   ├── version.json        # Aktuelle Version und Dateiname
│   └── MikoTec-LED-Uhr_vX.X.bin
└── platformio.ini
```

## Webinterface

Die Uhr ist im lokalen Netzwerk erreichbar unter:
- `http://mikotec-led-uhr.local` (mDNS)
- `http://[IP-Adresse]`

### Seiten

- **Startseite** – Farben, Helligkeit, Überblendeffekt, Farbschemata
- **Einstellungen** – Stundenmarkierungen, Schlafmodus, Zeitzone, Uhrentyp
- **Zeitzone** – Standort per Adresssuche, manueller UTC-Versatz oder Städteauswahl
- **Firmware Update** – Manuelle Firmware-Aktualisierung per Dateiupload (`/update`)
- **Hilfe** – Bedienungshinweise
- **Support** – Live-Log der Uhr

## Auto-Update System

Die Uhr prüft täglich einen HTTP-Server auf neue Firmware-Versionen:

1. `version.json` wird vom Server geladen
2. Versionsnummer wird mit der installierten Version verglichen
3. Bei neuer Version wird die `.bin`-Datei heruntergeladen und geflasht
4. Die Uhr startet automatisch mit der neuen Firmware neu

Ein Cronjob auf dem Update-Server synchronisiert die Dateien aus diesem GitHub-Repository.

## Schlafmodi

| Modus | Beschreibung |
|-------|-------------|
| Schwarz | Alle LEDs aus |
| Punkte | Nur Stunden- und Minutenpunkt sichtbar |
| Gedimmt | Komplette Uhr stark gedimmt |
| Mondphase | Aktuelle Mondphase als LED-Darstellung |
| Aus | Schlafmodus deaktiviert |

Die Nacht-Helligkeit ist separat einstellbar (0-100%).

## Versionierung

Das Versionierungsschema ist 4-stellig: `Major.Minor.Patch.Build` (z.B. `2.1.0.1`).

- **Build** wird bei jeder neuen `.bin`-Datei hochgezählt
- **Patch/Minor/Major** werden nach Absprache erhöht
- Nur **stable** Versionen werden automatisch per OTA verteilt
- Test-Versionen müssen manuell per OTA geflasht werden

### Stabile Versionen

- `v0.6-stable` – Erste stabile Version mit OTA und Auto-Schlaf
- `v1.5-stable` – Zeitstempel im Log, HTTP-OTA, SSDP-Spam entfernt
- `v2.0-stable` – Slider-Wertanzeige, Speicher-Tracking, Nacht-Helligkeit, Mondphasen-Fix

### Bekannte Crash-Versionen (nicht flashen!)

- `v0.8-CRASH` – Zeitstempel mit snprintf
- `v0.9-CRASH` – Zeitstempel in DualPrint write()
- `v1.0-CRASH` – Basiert auf v0.9
- `v1.2-CRASH` – Template-basierter Zeitstempel
- `v1.4-CRASH` – logTS() mit direkten TimeLib-Aufrufen

## Kompilierung

### Arduino CLI

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 MikoTec-LED-Uhr/
```

### Benötigte Board-Packages

- `esp8266:esp8266` Version 3.1.2

### Benötigte Libraries

- NeoPixelBus by Makuna 2.8.4
- NTPClient 3.2.1
- (Time, WebSockets, NTP sind im `lib/`-Ordner enthalten)

## Lizenz

Dieses Projekt basiert auf "The Light Clock" und steht unter der GNU General Public License v3.
