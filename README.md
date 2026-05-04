# MikoTec LED Uhr

Eine ESP8266-basierte LED-Uhr mit NeoPixel-Ring, Webinterface, MQTT/Home Assistant Integration und automatischen OTA-Updates.

## Features

- **LED-Uhranzeige** mit einstellbaren Stunden- und Minutenfarben, Überblendeffekt und Helligkeit
- **Mondphase** als Nachtanzeige – astronomisch korrekte Berechnung
- **Sonnenaufgang-Simulation** – eine Stunde vor Sonnenaufgang simuliert die Uhr einen Dämmerungseffekt
- **Sonnenpunkt** – goldener LED-Punkt zeigt die aktuelle Sonnenposition zwischen Auf- und Untergang
- **Auto-Schlaf** – schaltet automatisch bei Sonnenuntergang ab und bei Sonnenaufgang wieder ein
- **4 Farbschemata** – speichert Stundenfarbe, Minutenfarbe, Überblendeffekt und Helligkeit
- **Webinterface** – alle Einstellungen über den Browser konfigurierbar
- **MQTT / Home Assistant** – vollständige Steuerung mit automatischer Discovery (15 Entities)
- **OTA-Updates** – Firmware und LittleFS per Browser oder automatisch (Stable/Beta Kanal)
- **NTP-Zeitsynchronisation** mit automatischer Sommerzeit
- **LittleFS** – HTML, CSS und JS aus dem Flash-Dateisystem
- **Interaktive Hilfe** mit Tooltip-Hotspots über Screenshots
- **Log-System** – Ringpuffer + LittleFS + Live-MQTT + Update-Logs

## Hardware

- **ESP8266** NodeMCU v2
- **WS2812B LED-Ring** (60 LEDs Standard, konfigurierbar)
- GPIO2 (D4) via UART1

## Projektstruktur

```
MikoTec-LED-Uhr/
├── MikoTec-LED-Uhr.ino    # Hauptdatei: Globals, Includes, setup(), loop()
├── m01_log.ino             # Ringpuffer, DualPrint Klasse
├── m02_timestamp.ino       # Zeitstempel-Cache, logTS()
├── m03_sun.ino             # Sonnenauf-/untergang Berechnung
├── m04_ota.ino             # OTA Update System (Stable/Beta)
├── m05_mqtt.ino            # MQTT Client, Discovery, State, Config
├── m06_config.ino          # EEPROM: loadConfig, writeInitialConfig
├── m07_wifi.ino            # WiFi, Access Point, mDNS
├── m08_server.ino          # Server Handler Setup
├── m09_pages.ino           # Web-Seiten: Startseite (handleRoot)
├── m10_nightcheck.ino      # Nachtmodus-Prüfung
├── m11_settings.ino        # Settings, Timezone Handler
├── m12_face.ino            # Clock Face Rendering, Alarm, Dawn, Moon
├── m13_misc.ino            # Diverse Handler, WebSocket, Game
├── h/                      # Header-Dateien (PROGMEM Fallback)
├── data/                   # LittleFS Dateien (HTML, CSS, JS)
├── images/                 # Screenshots für Hilfe-Seite
└── updates/                # Firmware + LittleFS Binaries für OTA
    └── version.json        # Stable/Beta Versionsinformation
```

## Webinterface

### Seiten

| Endpunkt | Methode | Beschreibung |
|----------|---------|-------------|
| `/` | GET | Startseite mit Uhr-Canvas und Farbwähler |
| `/index.html` | GET | Alias für Startseite |
| `/settings` | GET/POST | Einstellungen (Stundenmarken, Schlaf, Zeitzone, MQTT, Beta) |
| `/timezone` | GET | Zeitzone per Suche, manuell oder Stadtauswahl |
| `/hilfe` | GET | Interaktive Hilfe mit Tooltip-Hotspots |
| `/support` | GET | Systeminfo, Betriebszeit, serielles Protokoll |
| `/alarm` | GET | Alarm-Einstellungen |
| `/game` | GET | Easter-Egg Game |
| `/update` | GET | Firmware OTA-Upload per Browser |
| `/update_fs` | GET/POST | LittleFS-Image OTA-Upload per Browser |

### JSON API

| Endpunkt | Methode | Beschreibung |
|----------|---------|-------------|
| `/getstate` | GET | Aktuelle Uhr-Werte: Zeit, Farben, Modus, Helligkeit, Farbschemata |
| `/getsettings` | GET | Alle Einstellungen: Pixel, Zeitzone, Schlaf, MQTT, Beta-Channel |
| `/getmqtt` | GET | MQTT-Konfiguration und Verbindungsstatus |
| `/getsysinfo` | GET | Systeminfo: Heap, Flash, Uptime, IP, MAC, Gateway |
| `/gettime` | GET | Aktuelle Zeit als `HH:MM:SS` |
| `/getlog` | GET | Aktueller Log (RAM-Ringpuffer) |
| `/getlog?prev=1` | GET | Log vom letzten Boot (LittleFS) |

### Steuerung

| Endpunkt | Methode | Beschreibung |
|----------|---------|-------------|
| `/setmqtt` | POST | MQTT-Einstellungen speichern (broker, port, user, pass, enabled) |
| `/timeset` | GET | Zeit manuell setzen (`?time=HH:MM:SS`) |
| `/reboot` | GET | ESP neu starten |
| `/nightmodedemo` | GET | Nachtmodus-Demo aktivieren |
| `/dawn` | GET | Dämmerungseffekt starten |
| `/moon` | GET | Mondphasen-Anzeige aktivieren |
| `/brighttest` | GET | Helligkeitstest |
| `/lightup` | GET | Alle LEDs aufleuchten lassen |
| `/reflection` | GET | Reflexionstest |
| `/speed` | GET | Speedup-Modus |
| `/cleareeprom` | GET | EEPROM-Reset-Bestätigungsseite |
| `/cleareepromsure` | GET | EEPROM tatsächlich löschen und neu starten |
| `/switchwebmode` | GET | Zwischen Normal- und AP-Modus wechseln |

### Statische Ressourcen

| Endpunkt | Beschreibung |
|----------|-------------|
| `/clock.js` | Canvas-Uhr JavaScript |
| `/Colour.js` | Farb-Klassen (RGB/HSV/HSL) |
| `/clockmenustyle.css` | Menü-Stylesheet |
| `/spectrum.css` | Spectrum Colorpicker CSS |
| `/spectrum.js` | Spectrum Colorpicker JS |
| `/description.xml` | SSDP/UPnP Gerätebeschreibung |

### Update-Logs

| Endpunkt | Beschreibung |
|----------|-------------|
| `/last_fw_update.txt` | Log direkt vor dem letzten Firmware-Update |
| `/last_fs_update.txt` | Log direkt vor dem letzten Dateisystem-Update |
| `/fs_ota_log` | Log des letzten FS-OTA-Vorgangs |

## MQTT / Home Assistant

Die Uhr wird über MQTT Discovery automatisch als Gerät in Home Assistant erkannt.

### Entities

| Entity | Typ | Beschreibung |
|--------|-----|-------------|
| Power | Switch | Uhr komplett an/aus |
| Helligkeit | Number | 10-100% |
| Modus | Select | normal / night / dawn |
| Sekunden | Switch | Sekundenzeiger an/aus |
| Sonnenpunkt | Switch | Sonnenpunkt an/aus |
| Stundenmarken | Select | Keine / Mittag / Quadranten / Stunden / Abdunkeln |
| Blendpoint | Number | 0-100% |
| Stundenfarbe | Text | Hex z.B. #ff0000 |
| Minutenfarbe | Text | Hex z.B. #0000ff |
| Firmware | Sensor | Aktuelle Version |
| Sonnenaufgang | Sensor | Berechnete Uhrzeit |
| Sonnenuntergang | Sensor | Berechnete Uhrzeit |
| Beta-Updates | Switch | Stable/Beta Kanal |
| Firmware Update | Update | Version + Install-Button |
| Log | Sensor | Letzte Log-Zeile |

### Topics

- `lightclock/<name>/state` – Status-JSON (alle 30s)
- `lightclock/<name>/set/#` – Steuerbefehle
- `lightclock/<name>/log` – Live-Log
- `lightclock/<name>/update_state` – Update-Status

## OTA Update System

### Auto-Update Zeiten
00:00, 03:00, 06:00, 09:00, 12:00, 15:00, 18:00, 21:00 Uhr + 30s nach Boot

### version.json Format
```json
{
  "stable_version": "2.3.0.7",
  "stable_file": "MikoTec-LED-Uhr_v2.3.0.7.bin",
  "stable_fs_version": "2.3.0.0",
  "stable_fs_file": "littlefs_v2.3.0.0.bin",
  "beta_version": "2.3.0.21",
  "beta_file": "MikoTec-LED-Uhr_v2.3.0.21.bin",
  "beta_fs_version": "2.3.0.17",
  "beta_fs_file": "littlefs_v2.3.0.17.bin"
}
```

### Update-Server
`http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/`

## Kompilierung

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 MikoTec-LED-Uhr/
```

### Libraries
- NeoPixelBus 2.8.4, PubSubClient 2.8, NTPClient 3.2.1, WebSockets 2.7.2, Time 1.6.1, ESP8266 Core 3.1.2

## Versionierung

Schema: `Major.Minor.Patch.Build` (z.B. `2.3.0.21`)

- **Build** wird bei jeder .bin hochgezählt
- **Patch** bei neuen Features
- **Minor/Major** nur nach Absprache

## Changelog

### v2.3 – MQTT / Home Assistant (01.-04.05.2026)
- v2.3.0.30: OOM-Fix im AP-Modus (WiFi-Setup) — chunked streaming statt großer Strings, neues LittleFS mit Datum-Eingabe
- v2.3.0.29: Manuelles Datum setzen in Settings (für korrekten Mondstand ohne NTP)
- v2.3.0.28: MQTT Device-Name aus clockname (Bindestriche → Leerzeichen in HA-Anzeige), README mit allen Handlern
- v2.3.0.27: NTP-Retry-Delay entfernt (Boot wieder schnell), nur noch 2 schnelle Versuche
- v2.3.0.26: Gateway und MAC-Adresse im Log ausgeben zur Diagnose
- v2.3.0.25: WiFi.persistent(false) — ESP holt sich immer frische DHCP-Lease statt gecachte IP
- v2.3.0.24: DNS-Fallback auf 8.8.8.8 wenn DHCP keinen DNS liefert, NTP Retry-Delay auf 1s erhöht
- v2.3.0.23: NTP Retry-Schleife (bis 5 Versuche), Helligkeit wird bei Settings-Save nicht mehr auf maxBrightness zurückgesetzt
- v2.3.0.22: OTA-Loop-Fix (Versionsstring stimmte nicht mit Dateiname überein), stable+beta auf 2.3.0.22
- v2.3.0.21: Refactoring in 13 Module (Hauptdatei 726 Zeilen)
- v2.3.0.20: Update-Check zu festen Uhrzeiten (alle 3h)
- v2.3.0.19: Update-Logs als downloadbare Dateien
- v2.3.0.18: Fix Beta-Channel in Settings-Handler
- v2.3.0.11-17: Hilfe-Seite mit Tabs und Tooltip-Hotspots
- v2.3.0.8-10: Stable/Beta Update-Kanal
- v2.3.0.5-7: Live-Log, Update Entity, Auto LittleFS-Update
- v2.3.0.2-4: Device mit Entities statt Light, Power Fix
- v2.3.0.0-1: MQTT Integration, Discovery, Buffer Fix

### v2.2 – LittleFS (25.-29.04.2026)
- LittleFS Dateisystem, OTA FS-Upload, Log-Persistenz, Settings Redesign

### v2.1 – Sonnenpunkt (21.04.2026)
- Sonnenpunkt, Menü-Overlay, Timezone-Redesign, Open-Meteo API

### v2.0 – Stabilisierung (21.04.2026)
- Slider, Nacht-Helligkeit, Mondphasen-Fix, Zeitstempel

### v1.5 – OTA (20.04.2026)
- HTTP-OTA, Zeitstempel-Fix

### v0.6 – Auto-Schlaf (19.04.2026)
- Auto-Schlaf, OTA Auto-Update

### v0.1 – Erstes Release (19.04.2026)
