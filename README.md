# MikoTec LED Uhr

Eine ESP8266-basierte LED-Uhr mit NeoPixel-Ring, Webinterface und automatischen Updates.

## Features

- **LED-Uhranzeige** mit einstellbaren Stunden- und Minutenfarben, Überblendeffekt und Helligkeit
- **Mondphase** als Nachtanzeige – astronomisch korrekte Berechnung
- **Sonnenaufgang-Simulation** – Uhr simuliert eine Stunde vor Sonnenaufgang einen langsamen Dämmerungseffekt
- **Auto-Schlaf** – schaltet automatisch bei Sonnenuntergang ab und bei Sonnenaufgang wieder ein
- **Nacht-Helligkeit** – separate Helligkeitseinstellung für den Schlafmodus
- **4 Farbschemata** – speichere und lade verschiedene Farbkombinationen
- **Webinterface** – alle Einstellungen über den Browser konfigurierbar
- **OTA-Updates** – Firmware und LittleFS-Dateisystem per Browser aktualisierbar
- **Automatische Updates** – Uhr prüft täglich einen Update-Server und installiert neue Versionen
- **NTP-Zeitsynchronisation** – automatische Uhrzeit über das Internet
- **Sommerzeit** – automatische DST-Erkennung
- **LittleFS** – HTML, CSS und JS werden aus dem Flash-Dateisystem geladen (RAM-Entlastung)

## Hardware

- **ESP8266** (NodeMCU v2)
- **NeoPixel WS2812B LED-Ring** (60 LEDs Standard)
- Anschluss über GPIO2 (D4) via UART1

## Projektstruktur

```
MikoTec-LED-Uhr/
├── MikoTec-LED-Uhr.ino    # Hauptsketch
├── h/                      # Header-Dateien (PROGMEM Fallback)
├── data/                   # LittleFS Dateien (HTML, CSS, JS)
├── html/                   # Entwicklungs-Kopie HTML
├── css/                    # Entwicklungs-Kopie CSS
├── js/                     # Entwicklungs-Kopie JS
├── updates/                # Firmware-Binaries und LittleFS-Images für OTA
│   ├── version.json        # Aktuelle stabile Version
│   ├── MikoTec-LED-Uhr_vX.X.X.XX.bin
│   └── littlefs_vX.X.X.XX.bin
└── platformio.ini
```

## Webinterface

- `http://mikotec-led-uhr.local` (mDNS)
- `http://192.168.10.83`

### Endpunkte

| Endpunkt | Beschreibung |
|----------|-------------|
| `/` | Startseite mit Uhr-Canvas |
| `/settings` | Einstellungen |
| `/timezone` | Zeitzone |
| `/update` | Firmware OTA-Upload |
| `/update_fs` | LittleFS-Image OTA-Upload |
| `/getstate` | JSON: aktuelle Uhr-Werte |
| `/getlog` | Aktueller Log (RAM-Buffer) |
| `/getlog?prev=1` | Log vom letzten Boot (LittleFS) |
| `/log_prev.txt` | Log-Datei direkt downloadbar |

## OTA Update System

### Firmware
Über `/update` per Browser oder automatisch täglich vom Update-Server.

### LittleFS-Dateisystem
Über `/update_fs` per Browser. Nur nötig wenn sich HTML/CSS/JS geändert haben.
Nach dem Upload: Log herunterladen → Neustart-Button klicken.

### Auto-Update Server
`http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/`

## Versionierung

Schema: `Major.Minor.Patch.Build` (z.B. `2.2.0.12`)

- **Build** wird bei jeder neuen `.bin`-Datei hochgezählt
- **Minor/Major** nur nach expliziter Absprache
- Nur **stable** Versionen werden automatisch per OTA verteilt

### Stabile Versionen

- `v2.1.0.15` – Letzte stabile Version vor LittleFS-Migration
- `v2.0-stable` – Slider, Speicher-Tracking, Nacht-Helligkeit, Mondphasen-Fix
- `v1.5-stable` – HTTP-OTA, Zeitstempel-Fix
- `v0.6-stable` – Erste stabile Version

### Bekannte Crash-Versionen (nicht flashen!)

- `v0.8-CRASH` bis `v1.4-CRASH` – Verschiedene Zeitstempel-Crashes
- `v2.1.0.16` bis `v2.1.0.19` – Instabil, zurückgerollt

## Kompilierung

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 MikoTec-LED-Uhr/
```

### Benötigte Libraries
- NeoPixelBus by Makuna 2.8.4
- NTPClient 3.2.1
- WebSockets 2.7.2
- Time 1.6.1
- ESP8266 Core 3.1.2

## Changelog

### v2.2.0.25 (26.04.2026)
- Fix: Inline-Scripts aus settings.html entfernt — toggleSleepFields() wurde vor settings.js geladen und verursachte ReferenceError der das gesamte JS stoppte
- Fix: Alle JS-Funktionen in settings.js, alles läuft unter DOMContentLoaded
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.25.bin

### v2.2.0.24 (26.04.2026)
- Debug: /getsettings loggt jetzt den kompletten JSON-Response (zur Diagnose Checkbox-Problem)
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.24.bin

### v2.2.0.23 (26.04.2026)
- Fix: powerType wird jetzt in handleSettings korrekt verarbeitet (Formular schickt powerType, nicht maxbright)
- Fix: timezone wird jetzt in handleSettings gespeichert
- Nur Firmware-Update nötig, kein neues LittleFS-Image

### v2.2.0.22 (26.04.2026)
- Fix: Formular-Submit auf POST umgestellt (GET-URL war zu lang für ESP8266 → Parameter wurden abgeschnitten)
- Fix: /settings für HTTP_GET und HTTP_POST registriert
- Debug: alle Args werden beim Submit geloggt (zur Diagnose)
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.22.bin

### v2.2.0.21 (26.04.2026)
- Fix: Checkboxen werden jetzt korrekt gesetzt (parseInt statt ==1 Vergleich)
- Fix: sleep/sleepmin/wake/wakemin in /getsettings direkt aus EEPROM (nicht aus Laufzeit-Variable die AutoSleep überschreibt)
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.21.bin

### v2.2.0.20 (26.04.2026)
- Fix: Settings-Seite exakt nach v2.1.0.20-Vorlage wiederhergestellt (Layout, Zeitzone-Dropdown, DST-Checkbox, Zeit-setzen-Button)
- Fix: timezonevalue und DSTtime in /getsettings ergänzt
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.20.bin

### v2.2.0.19 (26.04.2026)
- Fix: Settings-Seite Layout wiederhergestellt (Slider für Nacht-Helligkeit, time-Felder für Schlafzeiten, Autosleep-Toggle)
- Fix: sleep/wake werden wieder als type=time Felder (HH:MM) verarbeitet
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.19.bin

### v2.2.0.18 (26.04.2026)
- Fix: /getsettings Endpunkt hinzugefügt — Settings-Seite wurde leer angezeigt (LittleFS-Architektur)
- Fix: sleep/sleepmin und wake/wakemin werden jetzt korrekt als separate Felder verarbeitet
- Fix: pixelCount und maxbright werden jetzt in handleSettings gespeichert
- Nur Firmware-Update nötig, kein neues LittleFS-Image

### v2.2.0.17 (26.04.2026)
- Neu: /getstate liefert 4 Farbschemata aus EEPROM als schemes-Array
- Scheme-Buttons zeigen Farbverlauf Stundenfarbe -> Minutenfarbe aus EEPROM
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.17.bin

### v2.2.0.16 (26.04.2026)
- Fix: Farb-Picker input type=color durch klickbaren Farbblock (Swatch) ersetzt
- Swatch zeigt aktuelle Farbe sofort nach getstate-fetch an
- Klick auf Swatch oeffnet nativen Color-Picker
- Swatch wird bei Farbwahl aktualisiert
- ⚠️ Nur neues LittleFS-Image erforderlich: littlefs_v2.2.0.16.bin

### v2.2.0.15 (26.04.2026)
- Fix: clock.js liest Startfarben nicht mehr aus hidden inputs - direkt als RGBColour gesetzt
- Fix: Farb-Picker Zuweisung in index.html bereinigt
- ⚠️ Nur neues LittleFS-Image erforderlich: littlefs_v2.2.0.15.bin

### v2.2.0.14 (26.04.2026)
- Fix: RGBColour nicht definiert - colour.js aus colourjs.h extrahiert und in data/ hinzugefuegt
- index.html: colour.js wird vor clock.js geladen
- Firmware: serveStatic fuer colour.js registriert
- ⚠️ Firmware UND neues LittleFS-Image erforderlich: littlefs_v2.2.0.14.bin

### v2.2.0.13 (26.04.2026)
- Fix: clock.js jQuery(document).ready() durch natives DOMContentLoaded ersetzt
- Fix: Spectrum Color Picker entfernt — index.html nutzt native input type="color"
- Fix: $("#canvas")[0] durch document.getElementById("canvas") ersetzt
- Kein jQuery mehr erforderlich auf der Startseite
- ⚠️ Neues LittleFS-Image erforderlich: littlefs_v2.2.0.13.bin (clock.js geändert)

### v2.2.0.12 (25.04.2026)
- Fix: clock.js aktualisiert nach /getstate fetch die Spectrum Color Picker korrekt
- Fix: hidden inputs hourcolor/minutecolor werden mit Werten aus getstate befüllt
- Fix: Brightness Slider und Firmware-Version werden nach fetch gesetzt
- ⚠️ Neues LittleFS-Image erforderlich: littlefs_v2.2.0.12.bin (clock.js geändert)

### v2.2.0.11 (25.04.2026)
- Fix: handleRoot, handleSettings, handleTimezone streamen sofort aus LittleFS
- Kein PROGMEM-String mehr aufgebaut wenn LittleFS-Datei vorhanden — spart mehrere KB RAM
- [WARN] Root-Seite unvollständig entfällt wenn LittleFS korrekt geflasht ist
- ⚠️ Neues LittleFS-Image erforderlich: littlefs_v2.2.0.11.bin

### v2.2.0.10 (25.04.2026)
- Neu: FS-OTA Erfolgsseite zeigt zwei Buttons: Log herunterladen + Uhr neu starten
- Log-Snapshot wird vor Neustart gesichert und als fs_ota_log.txt zum Download angeboten
- Neustart erst auf Knopfdruck — kein Log geht mehr verloren

### v2.2.0.9 (25.04.2026)
- Fix: Log-Datei wird vor LittleFS.end() geflusht und geschlossen (logFSready=false)
- Fix: ERR_CONNECTION_RESET nach FS-OTA — delay von 500ms auf 2000ms erhöht
- Neu: Erfolgsseite nach FS-OTA mit meta-refresh nach 10s zur Startseite

### v2.2.0.8 (25.04.2026)
- Fix: logFile.flush() aus logAppend() entfernt — blockierte den ESP beim FS-OTA Upload
- Log wird jetzt alle 5 Sekunden im loop() geflusht und vor ESP.restart()

### v2.2.0.7 (25.04.2026)
- Neu: Log-Persistenz in LittleFS — alle Log-Ausgaben werden in /log.txt geschrieben
- Beim Neustart wird /log.txt zu /log_prev.txt umbenannt
- /getlog?prev=1 liefert den Log vom letzten Boot aus LittleFS
- /log_prev.txt direkt im Browser downloadbar

### v2.2.0.6 (25.04.2026)
- Fix: LittleFS OTA-Upload — kein Vorab-Löschen aller Sektoren mehr beim Start
- Sektoren werden einzeln direkt vor dem Beschreiben gelöscht (verhindert ~45s Blockierung)

### v2.2.0.5 (25.04.2026)
- Fix: LittleFS OTA-Upload komplett neu implementiert mit ESP.flashEraseSector() und ESP.flashWrite()
- Kein Update-Mechanismus mehr (der Magic Byte 0xE9 erwartet hatte)
- Partition-Adresse aus Linker-Symbol _FS_start — immer korrekt

### v2.2.0.4 (25.04.2026)
- Fix: Update.end(false) — LittleFS-Image hat keinen MD5
- Bessere Fehlerausgabe: Update.getError() im Log

### v2.2.0.3 (25.04.2026)
- Fix: LittleFS Update.begin() Größe auf 0x1FA000 korrigiert

### v2.2.0.2 (25.04.2026)
- Fix: Update.begin() für LittleFS mit Partitionsgröße 0x200000

### v2.2.0.1 (25.04.2026)
- Neu: /update_fs Endpunkt für OTA-Upload des LittleFS-Dateisystems per Browser

### v2.2.0.0 (25.04.2026)
- Major: LittleFS Dateisystem integriert — CSS und JS aus dem Flash
- Alle HTML/CSS/JS Dateien in data/ Ordner
- /getstate erweitert: hourcolor, minutecolor (Hex), maxBrightness, fw, alarmactive
- Neuer /getsettings Endpunkt für settings.js

### v2.1.0.20 (Test)
- Sauberer Merge aus v2.1.0.15 mit allen Fixes aus v2.1.0.16–v2.1.0.19
- Fix: EEPROM Default timezone 34 (UTC+1/CET)
- Fix: Timezone-Dropdown per JS statt 82 String-Operationen
- Neu: Open-Meteo API für Sonnenzeiten

### v2.1.0.19 (INSTABIL)
- Sonnenzeiten-API auf Open-Meteo umgestellt

### v2.1.0.18 (INSTABIL)
- Fix: Heap-Problem durch Timezone-Dropdown Replace-Schleife

### v2.1.0.17 (INSTABIL)
- Sonnenzeiten API integriert (sunrise-sunset.org)

### v2.1.0.16 (INSTABIL)
- Fix: Heap-Überlauf in handleRoot durch Menü-Einbindung

### v2.1.0.15 (stabil)
- Fix: $externallinks in handleTimezone und handleClearRomSure

### v2.1.0.14
- timezone.h komplett neu im MikoTec-Design

### v2.1.0.13
- Fix: Menü-Button Einstellungen war kein klickbarer Link
- support.h: Log-Intervall einstellbar (5s–5min)

### v2.1.0.12
- Menü-Overlay auf allen Seiten aktiviert

### v2.1.0.11
- Rebuild auf Basis v2.1.0.10

### v2.1.0.10
- Sonnenpunkt in JS-Uhr der Startseite
- /getstate liefert sunriseMinutes und sunsetMinutes

### v2.1.0.9
- Fix: Sonnenpunkt-Richtung korrigiert

### v2.1.0.8
- Neu: Sonnenpunkt-Feature (goldener LED-Punkt, Sonnenposition tagsüber)

### v2.1.0.7
- Fix: nightCheck() vor NTP-Sync entfernt

### v2.1.0.6
- Fix: Settings leitet nach Speichern auf /settings zurück

### v2.1.0.5
- Log-Buffer von 8KB auf 6KB reduziert

### v2.1.0.4
- Heap-Check beim Aufbau der Root-Seite mit Warnung im Log

### v2.1.0.3
- Fix: Nacht-Helligkeit Replace fehlte in handleSettings

### v2.1.0.2
- Fix: Nacht-Helligkeit Slider zeigte Platzhalter statt Wert

### v2.1.0.1
- 4-stelliges Versionierungsschema eingeführt
- Update-Intervall auf 4h geändert

### v2.1 (2026-04-21)
- Mondphase-LEDs mit Nacht-Helligkeit skaliert

### v2.0-stable (2026-04-21)
- Dezente Wertanzeige neben Slidern
- Speicher-Tracking im Log
- Nacht-Helligkeit separat einstellbar
- Mondphasen-Berechnung korrigiert
- Zeitstempel für alle Log-Einträge
- SSDP-Spam entfernt

### v1.5-stable (2026-04-20)
- Zeitstempel mit globalem Cache
- OTA auf HTTP-Server umgestellt

### v1.3 (2026-04-20)
- OTA-URL auf lokalen HTTP-Server umgestellt

### v0.6-stable (2026-04-19)
- Auto-Schlaf mit Sonnenuntergang/Sonnenaufgang
- OTA Auto-Update Funktion

### v0.1 (2026-04-19)
- Erstes Release im GitHub Repository
- Umbenennung von "The Light Clock" zu "MikoTec LED Uhr"
