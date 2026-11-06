# Fleet-Telemetrie (Server-Seite)

Zentrales Monitoring der MikoTec-LED-Uhren-Flotte. **Variante B:** Die Uhren
schicken einen schlanken HTTP-Heartbeat an den bestehenden, öffentlich
erreichbaren nginx; ein interner Dienst prüft einen geteilten Schlüssel und
schreibt die Werte **intern** in InfluxDB.

> **Secrets gehören NICHT in dieses Repo.** Geteilter Firmware-Key (`SHARED_KEY`)
> und InfluxDB-Write-Token liegen ausschließlich auf dem Server unter
> `/etc/uhr-telemetry/config.env`. Die `*.example`-Dateien enthalten nur Platzhalter.
> In der Firmware steckt der geteilte Key in `secrets.h` (ebenfalls nicht im Repo).

## Datenfluss

```
Uhr (Fremdnetz) --HTTP POST /telemetry--> nginx (update-server :80, öffentlich)
      --> 127.0.0.1:8099 (Telemetrie-Dienst) --intern--> InfluxDB (192.168.10.25:8086)
```

Die Uhren reden **nie** direkt mit der DB. Port 8086 bleibt rein intern. Die
Uhren nutzen den ohnehin offenen Port 80 (gleicher wie `/updates/`), also
**keine neue FRITZ!Box-Freigabe** nötig.

## Identität vs. Authentifizierung

- **Identität (`device_id`):** die **ESP-Chip-ID** (`ESP.getChipId()` als Hex) –
  unveränderlich, eindeutig pro Chip, nicht konfigurierbar. Sie kommt im Request
  und sorgt dafür, dass jede Uhr dauerhaft **genau ein Gerät** in der DB ist.
- **Authentifizierung:** ein **geteilter Schlüssel** (`SHARED_KEY`, Header
  `X-Device-Key`), für alle Uhren gleich. Beweist nur „echte MikoTec-FW".
- **Zugriffskontrolle:** Allow-/Blocklist der ESP-IDs in `devices.json`
  (die eigentliche, scharfe Kontrolle). `allow` leer → alle außer `block`
  (Blacklist); `allow` gefüllt → nur diese IDs (Whitelist).

## API

`POST /telemetry`
- Header: `X-Device-Key: <geteilter Schlüssel>`
- Body (JSON): `device_id`, `fw_version`, `uptime`, `free_heap`, `reset_reason`, `crash_info`
- Antworten: `204` ok · `400` Body/`device_id` ungültig · `401` Key falsch · `403` Gerät nicht erlaubt · `405` kein POST · `502` InfluxDB-Fehler

Die `device_id` wird gegen `[0-9A-Za-z_-]{1,32}` validiert. Es werden
ausschließlich diese Felder verarbeitet, keine WLAN-/Standortdaten
(Datensparsamkeit / DSGVO).

## InfluxDB-Datenmodell

- Measurement: `heartbeat`
- Tags: `device_id`, `fw_version`, `reset_reason`
- Fields: `uptime` (int), `free_heap` (int), `crash_info` (string)

## Dateien auf dem Server (nicht im Repo, da teils Secrets)

| Pfad | Inhalt | im Repo |
|------|--------|---------|
| `/opt/uhr-telemetry/server.py` | Dienst (secret-frei) | `server.py` |
| `/etc/systemd/system/uhr-telemetry.service` | systemd-Unit | `uhr-telemetry.service` |
| `/etc/uhr-telemetry/config.env` | InfluxDB-Ziel + **Write-Token** + **SHARED_KEY** | `config.env.example` |
| `/etc/uhr-telemetry/devices.json` | **Allow-/Blocklist** der ESP-IDs | `devices.json.example` |
| nginx `location = /telemetry` | im Port-80-Block von `/etc/nginx/sites-available/default` | — |

## Allow-/Blocklist-Verwaltung

`/etc/uhr-telemetry/devices.json` (pro Request frisch gelesen → **ohne Neustart**):

```json
{ "allow": ["<esp-id>", "..."], "block": ["<esp-id>"] }
```

- **Uhr zulassen (Whitelist):** ESP-ID in `allow` eintragen. Sobald `allow`
  mindestens einen Eintrag hat, kommen nur noch gelistete IDs durch.
- **Uhr sperren:** ESP-ID in `block` eintragen (gewinnt immer).
- Die ESP-ID (`device_id`) steht im Uhren-Log (`[TELEMETRY] device_id: …`) und
  erscheint als Tag in InfluxDB.

## Setup (Kurzfassung)

1. **InfluxDB Write-only-Token** (nur Bucket `uhren`):
   ```
   influx auth create --org mikotec \
     --write-bucket "$(influx bucket list --org mikotec --name uhren --hide-headers | awk '{print $1}')" \
     --description "Heartbeat-Write Uhren-Flotte (nginx)"
   ```
2. **Geteilten Key** erzeugen (`openssl rand -hex 24`), in `config.env` als
   `SHARED_KEY` eintragen **und** in die Firmware-`secrets.h` (`TELEMETRY_SHARED_KEY`).
3. **Dienst**: System-User `uhrtel`, `server.py` nach `/opt/uhr-telemetry/`,
   `config.env` + `devices.json` nach `/etc/uhr-telemetry/` (`640`, `root:uhrtel`),
   Unit installieren, `systemctl enable --now uhr-telemetry`.
4. **nginx**: `location = /telemetry { proxy_pass http://127.0.0.1:8099; ... }` in
   den Port-80-Block (vor der 301-Redirect-Regel), `nginx -t && systemctl reload nginx`.

## Sicherheit

- Geteilter Key = Vorhürde; die **Allowlist der ESP-IDs ist die scharfe Kontrolle**.
- Write-only-Token (nur Bucket `uhren`), InfluxDB (8086) nur intern.
- Dienst läuft als unprivilegierter User `uhrtel` mit systemd-Hardening.
- `device_id` wird validiert (keine Line-Protocol-Injection).

## Firmware-Seite

`m15_telemetry.ino` (ab FW 2.3.0.64): Heartbeat ~30 s nach Boot, danach alle
15 min, `device_id = ESP.getChipId()`, geteilter Key aus `secrets.h`.

## Offen

- **Grafana**: InfluxDB-Datasource auf `192.168.10.25:8086` (eigener Read-Token) + Dashboard.
