# MikoTec LED Uhr

Eine ESP8266-basierte LED-Uhr mit NeoPixel-Ring, Webinterface, MQTT/Home Assistant Integration und automatischen OTA-Updates.

## Features

- **LED-Uhranzeige** mit einstellbaren Stunden- und Minutenfarben, Überblendeffekt und Helligkeit
- **Mondphase** als Nachtanzeige – astronomisch korrekte Berechnung nach der Meeus-Formel (Astronomical Algorithms, Kap. 48), Beleuchtung und Richtung aus dem Phasenwinkel
- **Sonnenaufgang-Simulation** – eine Stunde vor Sonnenaufgang simuliert die Uhr einen Dämmerungseffekt
- **Sonnenpunkt** – goldener LED-Punkt zeigt die aktuelle Sonnenposition zwischen Auf- und Untergang
- **Auto-Schlaf** – schaltet automatisch bei Sonnenuntergang ab und bei Sonnenaufgang wieder ein
- **Wecker/Alarm-System** – bis zu 8 Alarme wie bei einem Handy-Wecker: Uhrzeit, Wochentage (einzeln + Schnellwahl Mo–Fr/Wochenende/täglich), Lichteffekt und Dauer pro Alarm; danach zurück zur normalen Uhr
- **4 Farbschemata** – speichert Stundenfarbe, Minutenfarbe, Überblendeffekt und Helligkeit
- **Webinterface** – alle Einstellungen über den Browser konfigurierbar
- **MQTT / Home Assistant** – vollständige Steuerung mit automatischer Discovery (15 Entities)
- **OTA-Updates** – Firmware und LittleFS per Browser oder automatisch über HTTPS (TLS 1.2, Port 8443), Stable/Beta Kanal
- **Einstellbarer täglicher Neustart** – Uhrzeit in den Einstellungen konfigurierbar (Default 3:30 Uhr), prüft beim Hochfahren auf Updates
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
├── m11_settings.ino        # Settings, Timezone Handler, Neustart-Zeit
├── m12_face.ino            # Clock Face Rendering, Dawn, Moon (Meeus)
├── m13_misc.ino            # Diverse Handler, WebSocket, Game, Alarm-JSON
├── m14_alarm.ino           # Wecker-/Alarm-System (Effekte, EEPROM, Prüfung)
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
| `/alarm` | GET/POST | Wecker-Seite (bis zu 8 Alarme); POST speichert die Alarme |
| `/game` | GET | Easter-Egg Game |
| `/update` | GET | Firmware OTA-Upload per Browser |
| `/update_fs` | GET/POST | LittleFS-Image OTA-Upload per Browser |

### JSON API

| Endpunkt | Methode | Beschreibung |
|----------|---------|-------------|
| `/getstate` | GET | Aktuelle Uhr-Werte: Zeit, Farben, Modus, Helligkeit, Farbschemata |
| `/getsettings` | GET | Alle Einstellungen: Pixel, Zeitzone, Schlaf, MQTT, Beta-Channel, Neustart-Zeit |
| `/getmqtt` | GET | MQTT-Konfiguration und Verbindungsstatus |
| `/getsysinfo` | GET | Systeminfo: Heap, Flash, Uptime, IP, MAC, Gateway |
| `/gettime` | GET | Aktuelle Zeit als `HH:MM:SS` |
| `/getalarms` | GET | Alle Wecker als JSON-Array (enabled, hour, minute, days, effect, duration) |
| `/getlog` | GET | Aktueller Log (RAM-Ringpuffer) |
| `/getlog?prev=1` | GET | Log vom letzten Boot (LittleFS) |

### Steuerung

| Endpunkt | Methode | Beschreibung |
|----------|---------|-------------|
| `/setmqtt` | POST | MQTT-Einstellungen speichern (broker, port, user, pass, enabled) |
| `/setalarms` | POST | Wecker speichern (Formularfelder `a<i>_enabled`, `a<i>_hour`, `a<i>_minute`, `a<i>_days`, `a<i>_effect`, `a<i>_duration`) |
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

### Auto-Update
Der OTA-Check läuft beim Hochfahren in `initWiFi()` direkt nach dem WLAN-Connect und **vor** mDNS, SSDP und WebServer – an dieser Stelle ist der freie Heap maximal (~18 KB). Seit v2.3.0.52 läuft der OTA über **HTTP Port 80** (`WiFiClient`, kein TLS); das beseitigt den früheren Heap-Engpass beim BearSSL-Handshake, der den Firmware-Download scheitern ließ (`-1 connection failed`). Für dauerhaft an der Wand hängende Uhren startet die Uhr sich täglich selbst neu (Uhrzeit einstellbar, Default 3:30 Uhr) und prüft dabei auf Updates. Der Neustart erfolgt nur bei Uptime > 1 h, um eine Reboot-Schleife zu verhindern. Die früheren festen 3-Stunden-Checks sind entfernt (liefen bei zu wenig Heap).

Firmware- und FS-Update lösen nur bei **echt neuerer** Remote-Version aus (`isVersionNewer`), nicht bei jeder Ungleichheit – das verhindert dauerhafte Update-/Neustartschleifen bei abweichender oder älterer FS-Version.

### version.json Format
```json
{
  "stable_version": "2.3.0.22",
  "stable_file": "MikoTec-LED-Uhr_v2.3.0.22.bin",
  "stable_fs_version": "2.3.0.0",
  "stable_fs_file": "littlefs_v2.3.0.0.bin",
  "beta_version": "2.3.0.54",
  "beta_file": "MikoTec-LED-Uhr_v2.3.0.54.bin",
  "beta_fs_version": "2.3.0.50",
  "beta_fs_file": "littlefs_v2.3.0.50.bin"
}
```

### Update-Server
`http://yzdlcru01ktmqlzy.myfritz.net/updates/` (HTTP Port 80, ohne TLS – seit v2.3.0.52)

Der OTA läuft über reines HTTP (`WiFiClient`), weil der BearSSL-TLS-Handshake auf dem ESP8266 zu viel Heap brauchte und der Firmware-Download dadurch scheiterte (`-1 connection failed`). Integrität und Authentizität werden stattdessen über die **Code-Signatur** sichergestellt (siehe unten), sodass HTTP ohne TLS unbedenklich ist. Der frühere TLS-Endpunkt auf Port 8443 bleibt serverseitig als Fallback bestehen.

### OTA-Code-Signatur (RSA-2048 + SHA256)
Der automatische OTA-Firmware-Download ist durch eine Code-Signatur geschützt: Nur mit dem privaten Schlüssel signierte Firmware wird akzeptiert. Der Public Key ist in die Firmware einkompiliert (`ota_pubkey`); die `.bin` wird beim Build mit `tools/signing.py` des ESP8266-Cores signiert. Der **private Schlüssel wird niemals ausgeliefert oder ins Repo eingecheckt**.

Die Verify-Objekte (`BearSSL::PublicKey` + `SigningVerifier`) werden **transient** nur um den Download herum angelegt und sofort wieder freigegeben (`Update.installSignature(nullptr,nullptr)` + `delete`) – dauerhaft würden sie ~6,5 KB Heap belegen und den WiFi-Stack zum Absturz bringen (Exception 29, behoben in v2.3.0.53).

**Geltungsbereich:** Die Prüfung greift beim **Auto-OTA-Firmware-Download** und – seit v2.3.0.54 – auch beim **Browser-Upload** (`/update`). Beim Browser-Upload werden die Verify-Objekte im `Update.onStart` transient angelegt und zuvor MQTT getrennt, da im Normalbetrieb nur ~6,5 KB Heap frei sind (`loop()` reconnectet MQTT automatisch). Nur der **USB-Flash** prüft *keine* Signatur (physischer Zugriff nötig).

**Verhalten bei fremder oder unsignierter Firmware:** Das Image wird zwar heruntergeladen, die Prüfung schlägt aber fehl, das Update wird verworfen und der Boot-Slot *nicht* aktiviert – die Uhr bleibt auf der laufenden Version:

```
[OTA] FEHLER: Firmware-Update fehlgeschlagen (12): Update error: ERROR[12]: Signature verification failed
```

**Signatur-Test (verifiziert):** Eine unsignierte `.bin` mit höherer Versionsnummer in `/updates/` ablegen, `version.json` (beta) darauf zeigen lassen – dabei den Update-Cron vorher pausieren, sonst überschreibt er die Teständerung von GitHub. Uhr neu starten. Erwartetes Ergebnis: `ERROR[12]: Signature verification failed`, die Uhr bootet unverändert mit der signierten Version weiter. Getestet am 26.10.2026 gegen v2.3.0.53.

## EEPROM-Belegung

512 Bytes. Wichtigste Adressen:

| Adresse | Inhalt |
|---------|--------|
| 0–31 | WLAN-SSID |
| 32–96 | WLAN-Passwort |
| 175–178 | Breiten-/Längengrad |
| 179 | Zeitzone |
| 181 | Stundenmarken |
| 182 / 183 | Schlaf Stunde / Minute |
| 185 | DST automatisch |
| 186 | webMode (Setup/Normal) |
| 189 / 190 | Wake Stunde / Minute |
| 191 | Helligkeit |
| 192 | Sommerzeit |
| 230 / 231 | Pixelanzahl / max. Helligkeit |
| 232 | Hemisphäre |
| 236 | MQTT aktiv |
| 237–238 | MQTT-Port |
| 239–366 | MQTT Broker / User / Passwort |
| 367 | Beta-Channel |
| **368** | **rebootHour** (täglicher Neustart, Default 3) |
| **369** | **rebootMinute** (Default 30) |
| **370–417** | **Alarme** (8 × 6 Bytes: enabled, hour, minute, days, effect, duration) |
| 500 | Magic Byte (0xC4 = eingerichtet) |

## Wecker/Alarm-System

- Bis zu **8 Alarme**, jeder einzeln aktivierbar (`m14_alarm.ino`).
- Pro Alarm: Uhrzeit, Wochentage als Bitmask (Bit0 = Montag … Bit6 = Sonntag), Effekt, Dauer in Minuten.
- **Effekte** (`effect`): `0` = kompletter strandtest (alle Effekte im Wechsel), `1` = colorWipe, `2` = theaterChase, `3` = Regenbogen.
- **Schnellwahl** der Tage: Mo–Fr (`31`), Wochenende (`96`), Täglich (`127`), Keine (`0`).
- Effekte sind **non-blocking** (kein `delay`/WDT-Reset), die Uhr bleibt responsiv. Nach Ablauf der Dauer schaltet die Uhr automatisch zurück in den Normalbetrieb.
- `checkAlarms()` prüft einmal pro Minute, `alarmRunning()` überwacht die Dauer, `alarmEffectShow()` ist die Effekt-Engine.

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

### v2.3.0.54 – Signaturprüfung auch beim Browser-Upload (Variante A) (26.10.2026)
- Die Code-Signatur gilt jetzt auch für den **Browser-Upload** (`/update`), nicht mehr nur für den Auto-OTA. Damit ist auch der lokale Upload-Weg gegen untergeschobene Firmware abgesichert.
- Heap-schonend umgesetzt: Im `Update.onStart` wird zuerst MQTT getrennt (gibt Heap frei, `loop()` reconnectet automatisch), dann werden die Verify-Objekte transient angelegt; Freigabe in `onEnd`/`onError`. Der Browser-Upload läuft im Normalbetrieb mit nur ~6,5 KB Heap, daher das Freimachen. Der tatsächlich verfügbare Heap wird als `[UPDATE] Signaturpruefung aktiv, freier Heap: …` geloggt.
- USB-Flash prüft weiterhin keine Signatur (physischer Zugriff).
- FW 2.3.0.54, FS bleibt 2.3.0.50.

### v2.3.0.53 – Signatur-Heap-Fix: Verify-Objekte nur transient (26.10.2026)
- Fix fuer den WiFi-Stack-Crash (Exception 29, StoreProhibited) aus 2.3.0.52: Die BearSSL-Signatur-Objekte (`PublicKey` + `SigningVerifier`) wurden dauerhaft im `setup()` angelegt und belegten ~6,5 KB Heap. Dadurch fehlte dem WiFi-Stack im MQTT-Discovery-Pfad der Speicher → Absturz im `sys`-Context → Bootschleife. Boot, Port 80 und Signatur selbst liefen korrekt – es war reiner Heap-Mangel (Heap beim OTA-Check 11,5 KB statt 18 KB).
- Lösung: Die Verify-Objekte werden jetzt **nur transient** in `checkForUpdate()` um den Auto-OTA-Download herum angelegt und direkt danach freigegeben (`Update.installSignature(nullptr,nullptr)` + `delete`). Im Normalbetrieb kein Heap-Verbrauch → voller Heap fuer WiFi/MQTT.
- Signaturpruefung damit **nur beim Auto-OTA-Firmware-Download** (MITM-Schutz ueber HTTP). Browser-Upload (lokal) und Auto-OTA-FS-Update bleiben ungeprueft. Volle Abdeckung inkl. Browser-Upload ist fuer spaeter vorgemerkt.
- FW 2.3.0.53, FS bleibt 2.3.0.50.

### v2.3.0.52 – OTA über HTTP Port 80 + Code-Signatur, Browser-Upload-Fix (26.10.2026)
- **Bootschleifen-Fix (Browser-Firmware-Upload):** Der `Update.onStart`-Callback in `m08_server.ino` sicherte beim Upload-Start den Log per `getLogContent()` + `/last_fw_update.txt` + MQTT-Chunks. Das drueckte den knappen Heap (~6,5 KB) so stark, dass der Flash-Vorgang mittendrin abbrach → korruptes Image → Bootschleife. Nur per USB geflashte .51 bootete sauber. Die Log-Sicherung im onStart ist jetzt entfernt (LittleFS ueberlebt das FW-Update ohnehin).
- **OTA über HTTP Port 80 statt HTTPS/8443:** `update_version_url`/`update_bin_base_url` auf `http://…/updates/`, in `m04_ota.ino` durchgaengig `WiFiClient` statt `BearSSL::WiFiClientSecure`. Kein TLS-Handshake mehr → kein Heap-Engpass beim Download (war die Ursache fuer `-1 connection failed`). Server liefert `/updates/` auf Port 80 nativ aus.
- **Code-Signatur (RSA-2048 + SHA256):** Public Key einkompiliert, `Update.installSignature()` aktiv fuer Auto-OTA UND Browser-Upload. Nur mit dem privaten Schluessel signierte Firmware wird akzeptiert → HTTP ohne TLS ist gegen Manipulation (MITM) abgesichert. Die `.bin` wird mit `signing.py` signiert. USB-Flash prueft keine Signatur.
- Hinweis: Ab dieser Version muessen auch kuenftige LittleFS-Images signiert werden (Auto-OTA-FS-Weg). FS hier unveraendert (2.3.0.50).
- FW 2.3.0.52, FS bleibt 2.3.0.50.

### v2.3.0.51 – OTA-Download-Heap-Fix (Scope-Kapselung) (25.10.2026)
- OTA-Firmware-Download scheiterte weiterhin mit `-1` (connection failed): Beim automatischen Neustart um 03:30 erkannte der Check die neue Version 2.3.0.50 korrekt (`HTTP 200`, version.json geladen), der anschliessende Download brach aber ab. Ursache: Der BearSSL-Client des `version.json`-Checks war eine lokale Variable in `checkForUpdate()` und lebte bis zum Funktionsende - sein TLS-Kontext (~12 KB) blieb belegt, waehrend darunter der Download-Client erstellt wurde. Dadurch standen vor dem Download nur noch ~5,7 KB Heap statt der ~15-18 KB fuer den TLS-Handshake.
- Fix: Der version.json-Check liegt jetzt in einem eigenen Scope-Block `{ }` in `m04_ota.ino`. Der Check-Client wird am Blockende zerstoert und sein Heap freigegeben, bevor der Firmware-/FS-Download-Client erstellt wird. `payload` wird vor dem Block deklariert und bleibt danach erhalten.
- Nur `m04_ota.ino` geaendert; keine Endpunkt-, EEPROM- oder data/-Aenderung, daher kein neues LittleFS-Image (FS bleibt 2.3.0.50).

### v2.3.0.50 – Wecker/Alarm-System mit Lichteffekten (24.10.2026)
- Neues Multi-Alarm-System (wie Handy-Wecker): bis zu 8 Alarme, jeder mit Uhrzeit, Wochentagen (einzeln + Schnellwahl Mo-Fr/Wochenende/taeglich), Effekt und Dauer. Gespeichert im EEPROM ab Adresse 370.
- Lichteffekte je Alarm waehlbar: kompletter strandtest (alle Effekte im Wechsel) oder einzeln colorWipe / theaterChase / Regenbogen. Non-blocking, laeuft die eingestellte Dauer, danach zurueck zur normalen Uhr.
- Neue Alarm-Seite (/alarm) im Design der uebrigen Oberflaeche, Web-Handler /getalarms und /setalarms.
- Einmaliges 24.10.-Testmodul entfernt (durch das Alarm-System ersetzt).
- Enthaelt den OTA-Download-Heap-Fix (2.3.0.49) und die Meeus-Mondphase (2.3.0.48).
- FW + FS auf 2.3.0.50. Finale Version.

### v2.3.0.49 – OTA-Download-Heap-Fix (24.10.2026)
- OTA-Firmware- und FS-Download scheiterten (-1, connection failed), weil die Log-Sicherung vor dem Download (getLogContent + MQTT-Log-Versand) den Heap von ~18 KB auf ~5,7 KB druckte - zu wenig fuer den zweiten TLS-Handshake. Diese Log-Sicherung an beiden Download-Stellen entfernt; das Log bleibt in /log_prev.txt erhalten (ueberlebt das Update).
- Enthaelt weiterhin Meeus-Mondphase und das strandtest-Testmodul (24.10. 6:45-7:15).
- FW + FS auf 2.3.0.49.

### v2.3.0.48 – Genauere Mondphase (Meeus) + strandtest-Test (23.10.2026)
- Mondphasen-Berechnung von der linearen Kosinus-Naeherung auf die Meeus-Formel (Astronomical Algorithms, Kap. 48) umgestellt: Beleuchtung und Richtung aus dem Phasenwinkel. Gegen ephem validiert: Max-Fehler 0,3 % (vorher bis 3,9 %, v.a. bei den Vierteln). now() wird vor der Rechnung auf UTC korrigiert (timezone + DSTtime).
- Testmodul (strandtest-Durchlauf am 24.10.2026, 6:45-7:15) bleibt fuer den OTA-Selbsttest enthalten.
- FW + FS auf 2.3.0.48. Testmodul wird in der finalen 2.3.0.50 entfernt.

### v2.3.0.47 – OTA-Selbsttest: strandtest-Durchlauf am 24.10.2026 (23.10.2026)
- Testmodul (m14_blinktest.ino) auf einen strandtest-Durchlauf umgestellt: colorWipe (rot/gruen/blau), theaterChase, rainbow, rainbowCycle, theaterChaseRainbow in Dauerschleife, am 24.10.2026 von 6:45 bis 7:15 Uhr. Non-blocking State-Machine (kein delay/WDT-Reset), damit die Uhr responsiv bleibt.
- FW + FS auf 2.3.0.47 (gleicher Stand), damit FW- und FS-OTA mitgetestet werden.
- Wird in der finalen Version (2.3.0.50) wieder entfernt.

### v2.3.0.46 – OTA-Selbsttest: buntes Blinken am 24.10.2026 (23.10.2026)
- Temporaeres Testmodul (m14_blinktest.ino): Am 24.10.2026 zwischen 6:45 und 7:15 Uhr blinkt die Uhr bunt (Regenbogen, an/aus). Sichtbarer Nachweis, dass das automatische OTA-Update (Firmware + LittleFS) ueber den naechtlichen Neustart funktioniert hat. Ausserhalb dieses Zeitfensters passiert nichts.
- FS auf 2.3.0.46 angehoben, damit auch das FS-OTA-Update mitgetestet wird.
- Wird in der finalen Version (2.3.0.50) wieder entfernt.

### v2.3.0.45 – OTA-Check vor mDNS/SSDP/WebServer (max. Heap) (23.10.2026)
- OTA-Boot-Check jetzt in initWiFi() direkt nach dem WLAN-Connect und VOR mDNS, SSDP und WebServer, wo der Heap maximal frei ist (~15 KB statt ~11 KB). Grund: SSL-Fehler -1000 (v2.3.0.44) zeigte, dass die TLS-Allokation an zu wenig Heap scheitert, nicht an der Record-Groesse.
- RX-Puffer wieder auf 512 (2048 in v2.3.0.44 war kontraproduktiv -> mehr Heap-Bedarf -> -1000).
- LittleFS.begin() vor dem Check vorgezogen (idempotent), damit der FS-Versionsvergleich korrekt ist und kein ungewolltes FS-Update ausloest.

### v2.3.0.44 – TLS-RX-Puffer 2048 + SSL-Fehlerdiagnose (23.10.2026)
- RX-Puffer an allen drei OTA-Stellen von 512 auf 2048 erhoeht. 512 war zu klein fuer das TLS-Zertifikats-Record im Handshake (Let's-Encrypt-ECDSA-Kette ~1,5 KB) -> Verbindung scheiterte mit HTTP -1. Grund: der Heap nach dem WLAN-Connect betraegt nur ~11 KB (WiFi-Stack belegt ~9 KB), nicht die erhofften ~19 KB.
- Diagnose: bei Verbindungsfehler (httpCode < 0) wird jetzt der genaue BearSSL-Fehlercode (getLastSSLError) und der freie Heap geloggt.

### v2.3.0.43 – OTA-Check direkt nach WLAN, MFLN-Probe entfernt (23.10.2026)
- OTA-Boot-Check laeuft jetzt DIREKT nach dem WLAN-Connect (vor NTP, WebSocket, mDNS, MQTT), wo der Heap maximal frei ist (~19 KB). Zuvor lief er nach WebSocket/mDNS bei nur ~10 KB -> HTTP -1 (Verbindung scheiterte).
- MFLN-Probe (probeMaxFragmentLength) an allen drei OTA-Stellen entfernt. Sie baute direkt vor dem echten Connect eine zweite TLS-Verbindung auf und fragmentierte bei knappem Heap den Speicher. setBufferSizes(512,512) bleibt; BearSSL handelt MFLN automatisch aus.

### v2.3.0.42 – Einstellbare Neustart-Zeit (23.10.2026)
- Die Uhrzeit des taeglichen Neustarts ist jetzt in den Einstellungen konfigurierbar (Feld "Taeglicher Neustart"). Default 3:30 Uhr. Der Neustart selbst ist fest aktiv und kann nicht abgeschaltet werden.
- Neue EEPROM-Werte: 368 = rebootHour, 369 = rebootMinute (Default 3:30 bei unbeschriebenem EEPROM). Geladen in loadConfig, gespeichert via handleSettings, ausgeliefert via /getsettings.
- UI: neues Zeitfeld in settings.html + Laden in settings.js (neues LittleFS-Image 2.3.0.42).
- FS-Version auf 2.3.0.42 angehoben = gleicher Stand wie die Firmware.

### v2.3.0.41 – OTA-Check vor MQTT + taeglicher Neustart (23.10.2026)
- OTA-Boot-Check laeuft jetzt in `setup()` VOR der MQTT-Initialisierung (~19 KB freier Heap statt ~7 KB), damit der TLS-Handshake nicht mehr in OOM laeuft. OTA wieder aktiv (`otaEnabled = true`).
- Planmaessige 3-Stunden-Checks entfernt (liefen bei zu wenig Heap). Stattdessen taeglicher Selbst-Neustart um 3:30 Uhr: beim Hochfahren prueft die Uhr auf Updates (genug Heap). Reboot nur bei Uptime > 1 h, um eine Reboot-Schleife um 3:30 zu verhindern.
- Loest die OOM-Neustartschleife dauerhaft und behaelt die automatische Update-Pruefung fuer dauerhaft an der Wand haengende Uhren.

### v2.3.0.40 – Notfall: OTA-Check deaktiviert (23.10.2026)
- Notfall-Firmware gegen die OOM-Neustartschleife: OTA-Boot-Check und planmaessige Checks ueber das Flag `otaEnabled = false` deaktiviert, damit kein TLS-Handshake mehr bei nur ~7 KB freiem Heap ausgeloest wird. Die Uhr laeuft damit stabil (ohne Auto-Update). Der TLS-Handshake-OOM blieb trotz setBufferSizes(512,512)+MFLN bestehen, weil vor dem Check nur ~7 KB Heap frei sind (nach WiFi+MQTT-Discovery). Wird in 2.3.0.41 geloest (OTA-Check vor MQTT, ~19 KB Heap).

### v2.3.0.39 – OTA TLS-Fix / OOM-Schleife behoben (22.10.2026)
- OTA-Update-URLs auf Port 8443 umgestellt (ESP8266-tauglicher TLS-1.2-Endpunkt des Update-Servers mit ssl_buffer_size 512).
- BearSSL an allen drei OTA-Stellen (version.json, Firmware-.bin, LittleFS-.bin) auf setBufferSizes(512, 512) + probeMaxFragmentLength(..., 8443, 512) umgestellt, damit der TLS-Handshake in den knappen Heap passt.
- MQTT-Client wird vor jedem TLS-Handshake getrennt (Heap-Peak entlasten); loop() reconnectet automatisch, Discovery wird nicht erneut gesendet.
- LittleFS-Download von nacktem WiFiClient (HTTP) auf BearSSL (HTTPS) umgestellt, passend zur HTTPS-Basis-URL.
- Ursache der Neustartschleife: Unhandled C++ exception OOM beim TLS-1.3-Handshake des OTA-Checks bei nur ~7,6 KB freiem Heap.

### v2.3.0.38 – OTA-Logik (22.10.2026)
- OTA-FS-Update loest nur noch bei ECHT neuerer Remote-FS-Version aus (statt bei jeder Ungleichheit). Neue Hilfsfunktion `isVersionNewer(remote, local)` vergleicht zwei 4-stellige Versionsstrings; `isNewerVersion` baut darauf auf. Verhindert dauerhaft Update-/Neustartschleifen bei abweichender oder aelterer FS-Version.

### v2.3.0.37 – Fixes (22.10.2026)
- LittleFS-Neustartschleife behoben: FS-Image `littlefs_v2.3.0.34.bin` neu gebaut, `data/fs_version.txt` auf 2.3.0.34 korrigiert und `version.json` (`beta_fs_version`/`beta_fs_file`) angeglichen. Ursache: das bisherige `littlefs_v2.3.0.33.bin` enthielt intern die FS-Version 2.4.0.0, wodurch der OTA-Vergleich (`!=`) bei jedem Boot erneut ausloeste.

### v2.4 – Mondphasen-Verbesserung (22.05.2026)
- v2.4.0.0: Mondposition wandert realistisch ueber den LED-Ring: Ost (zunehmend) -> Sued (Vollmond) -> West (abnehmend); Suedhalbkugel gespiegelt

### v2.3 – MQTT / Home Assistant (01.-04.05.2026)
- v2.3.0.33: Heap-Optimierung - Log-Buffer 4096, st-String leeren nach WiFi; neues LittleFS mit korrekter fs_version.txt
- v2.3.0.32: handleRoot sofortiger LittleFS-Stream ohne Args, WARN und doppelter LittleFS-Check entfernt
- v2.3.0.31: SSID und Passwort werden sauber eingelesen (kein Garbage/Nullbytes mehr im Log und WiFi-Connect)
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
