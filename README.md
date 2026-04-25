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

Die Uhr prüft alle 4 Stunden (6x täglich) einen HTTP-Server auf neue Firmware-Versionen:

1. `version.json` wird vom Server geladen
2. Versionsnummer wird mit der installierten Version verglichen (4-stellig: Major.Minor.Patch.Build)
3. Bei neuer Version wird die `.bin`-Datei heruntergeladen und geflasht
4. Die Uhr startet automatisch mit der neuen Firmware neu
5. Nur **stable** Versionen werden über `version.json` verteilt

Ein Cronjob auf dem Update-Server synchronisiert alle `.bin`-Dateien aus diesem GitHub-Repository. Die `version.json` wird nur aktualisiert wenn eine neue stable Version freigegeben wird.

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

## Changelog

### v2.1.0.17 (Test)
- Sonnenzeiten API integriert (api.sunrise-sunset.org / sonnenzeiten.org)
- Neue Funktion fetchSunriseSunset(): holt Sonnenauf- und -untergang täglich per HTTP, gecacht in apiSunriseMinutes/apiSunsetMinutes
- Neue Funktion getSunTimes(): liefert gecachte API-Werte oder Fallback auf lokale Berechnung
- Alle calcSunriseSunset()-Aufrufe (handleRoot, Loop, getstate) durch getSunTimes() ersetzt
- API-Abruf erfolgt einmal täglich zusammen mit NTP forceUpdate

### v2.1.0.16 (Test)
- Fix: Heap-Überlauf in handleRoot und handleSettings durch Menü-Einbindung
- $menu wird jetzt als erstes replace eingefügt (vor allen anderen replaces)
- toSend.reserve() in handleRoot (4096) und handleSettings (16384) zur Vermeidung von Heap-Fragmentierung

### v2.1.0.15 (Test)
- Fix: $externallinks wurde in handleTimezone und handleClearRomSure nicht ersetzt (CSS und Menü-Styling fehlten)

### v2.1.0.14 (Test)
- timezone.h: Komplett neu im MikoTec-Design, Bootstrap entfernt, eigenes Tab-System, Eingabefelder korrekt skaliert

### v2.1.0.13 (Test)
- Fix: Menü-Button "Einstellungen" war kein klickbarer Link (nur Text-Span)
- support.h: Log-Intervall von 5s auf 60s erhöht, Dropdown zur Intervallauswahl (5s/10s/30s/1min/5min/Aus)
- hilfe.h: ⓘ-Symbol entfernt, Hover-Tooltips direkt auf Wortnamen, Hilfetexte ausführlicher
- clearromsure.h: Ans globale Design angepasst (rcorners2, section-head, btn-Klassen)
- Update-Seite (/update): Ans Design angepasst, Menü eingebunden
- timezone.h: Eingabefelder mit box-sizing und max-width repariert (gingen über weißes div hinaus)

### v2.1.0.12 (Test)
- Menü-Overlay auf allen Seiten aktiviert ($menu Platzhalter + toSend.replace in allen Handlern)
- Alte btn-box Navigation entfernt aus root.h, settings.h, hilfe.h, support.h, timezone.h
- Nur noch Update-Button im Footer der Hauptseite
- Neustart-Button aus support.h entfernt (jetzt im Menü)
- Werksreset-Link aus settings.h entfernt (jetzt im Menü)

### v2.1.0.11 (Test)
- Keine Code-Änderungen – Rebuild auf Basis von v2.1.0.10 (Sonnenpunkt-Feature)

### v2.1.0.10 (Test)
- Sonnenpunkt in JS-Uhr der Startseite ergänzt (identisches Verhalten wie LED-Ring)
- `/getstate` liefert jetzt zusätzlich `sunriseMinutes` und `sunsetMinutes`

### v2.1.0.9 (Test)
- Sonnenpunkt-Richtung korrigiert: Aufgang bei 3 Uhr, gegen Uhrzeigersinn über 12 nach 9 Uhr (Untergang)

### v2.1.0.8 (Test)
- Sonnenpunkt-Feature: Zeigt die aktuelle Sonnenposition als goldenen LED-Punkt (nur tagsüber sichtbar)
- Neues Ein/Aus-Element in den Settings (Checkbox mit Tooltip)
- EEPROM-Adresse 235 für Sonnenpunkt-Einstellung
- Sonnenpunkt wird über `calcSunriseSunset()` berechnet (Breitengrad/Längengrad erforderlich)

### v2.1.0.7 (Test)
- Fix: nightCheck() vor NTP-Sync entfernt – verhinderte falsche Sonnenzeiten (doy=0 ergab vertauschte/falsche Werte)
- Sonnenaufgang-Simulation startet jetzt korrekt

### v2.1.0.6 (Test)
- Settings-Formular leitet jetzt nach dem Speichern auf /settings zurück statt auf / (Root-Seite) – verhindert Heap-Problem
- Settings Args-Verarbeitung in handleSettings verschoben

### v2.1.0.5 (Test)
- Log-Buffer von 8KB auf 6KB reduziert (8KB verursachte Heap-Probleme bei Root-Seite mit nur 8104 Bytes frei)

### v2.1.0.4 (Test)
- Settings: "Zurueck" Button zu "Hauptseite" umbenannt
- Heap-Check beim Aufbau der Root-Seite mit Warnung im Log
- yield() vor Root-Seiten-Aufbau für Heap-Freigabe

### v2.1.0.3 (Test)
- Fix: Nacht-Helligkeit Replace fehlte in handleSettings (war nur in handleRoot)
- Firmware-Version wird beim Start im Log ausgegeben

### v2.1.0.2 (Test)
- Fix: Nacht-Helligkeit Slider zeigte `$nightbrightness%` statt echtem Wert (Replace-Reihenfolge korrigiert)

### v2.1.0.1 (Test)
- 4-stelliges Versionierungsschema eingeführt
- Update-Intervall von 24h auf 4h geändert (6x täglich)
- Log-Buffer von 4KB auf 8KB verdoppelt
- `isNewerVersion()` unterstützt jetzt 4-stellige Versionen
- Auto-Update nur noch für stable Versionen

### v2.1 (2026-04-21)
- Mondphase-LEDs werden mit Nacht-Helligkeit skaliert

### v2.0-stable (2026-04-21)
- Dezente Wertanzeige neben Slidern auf der Startseite
- Speicher-Tracking im Log (Heap, Flash, Fragmentierung)
- Nacht-Helligkeit separat einstellbar (0-100%)
- Mondphasen-Berechnung korrigiert (Referenz-Neumond 6. Jan 2000)
- Zeitstempel `[DD.MM.YYYY HH:MM:SS]` für alle Log-Einträge
- SSDP-Spam entfernt
- nightCheck loggt nur noch bei geänderten Werten

### v1.5-stable (2026-04-20)
- Zeitstempel mit globalem Cache (kein DualPrint-Crash mehr)
- OTA auf HTTP-Server umgestellt (kein HTTPS/BearSSL)

### v1.3 (2026-04-20)
- OTA-URL auf lokalen HTTP-Server umgestellt
- Zeitstempel-Code entfernt (v0.8-v1.2 crashten)

### v0.6-stable (2026-04-19)
- Auto-Schlaf mit Sonnenuntergang/Sonnenaufgang
- Nominatim Geocoding statt GPS
- TimezoneDB API Key aktualisiert
- Timezone-Defaults auf Solingen (UTC+1)
- OTA Auto-Update Funktion (HTTPS/GitHub)

### v0.1 (2026-04-19)
- Erstes Release im GitHub Repository
- Umbenennung von "The Light Clock" zu "MikoTec LED Uhr"
