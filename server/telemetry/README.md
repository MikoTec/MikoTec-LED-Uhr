# Fleet-Telemetrie (Server-Seite)

Zentrales Monitoring der MikoTec-LED-Uhren-Flotte. **Variante B:** Die Uhren
schicken einen schlanken HTTP-Heartbeat an den bestehenden, öffentlich
erreichbaren nginx; der nimmt den POST an, ein interner Dienst authentifiziert
pro Uhr und schreibt die Werte **intern** in InfluxDB.

> **Secrets gehören NICHT in dieses Repo.** InfluxDB-Write-Token und Geräte-Keys
> liegen ausschließlich auf dem Server unter `/etc/uhr-telemetry/`. Die `*.example`-
> Dateien hier enthalten nur Platzhalter.

## Datenfluss

```
Uhr (Fremdnetz) --HTTP POST /telemetry--> nginx (update-server :80, öffentlich)
      --> 127.0.0.1:8099 (Telemetrie-Dienst) --intern--> InfluxDB (192.168.10.25:8086)
```

Die Uhren reden **nie** direkt mit der DB. Port 8086 bleibt rein intern (keine
Portfreigabe). Die Uhren nutzen den ohnehin offenen Port 80 (gleicher wie `/updates/`),
also **keine neue FRITZ!Box-Freigabe** nötig.

## Komponenten

- **update-server** (`192.168.10.20`, extern `http://yzdlcru01ktmqlzy.myfritz.net`):
  nginx + Telemetrie-Dienst.
- **UHR-DB-SRV** (`192.168.10.25`): InfluxDB 2.x, Org `mikotec`, Bucket `uhren`.

## API

`POST /telemetry`
- Header: `X-Device-Key: <geräte-key>`
- Body (JSON): `fw_version`, `uptime`, `free_heap`, `reset_reason`, `crash_info`
- Antworten: `204` ok · `400` Body ungültig · `401` Key ungültig · `403` nicht-POST · `502` InfluxDB-Fehler

Die `device_id` wird **serverseitig aus dem Key** bestimmt – eine Uhr kann keine
fremde ID behaupten. Es werden ausschließlich die sechs Felder verarbeitet,
keine WLAN-/Standortdaten (Datensparsamkeit / DSGVO).

## InfluxDB-Datenmodell

- Measurement: `heartbeat`
- Tags: `device_id`, `fw_version`, `reset_reason`
- Fields: `uptime` (int), `free_heap` (int), `crash_info` (string)

## Dateien auf dem Server (nicht im Repo, da teils Secrets)

| Pfad | Inhalt | im Repo |
|------|--------|---------|
| `/opt/uhr-telemetry/server.py` | Dienst (secret-frei) | `server.py` |
| `/etc/systemd/system/uhr-telemetry.service` | systemd-Unit | `uhr-telemetry.service` |
| `/etc/uhr-telemetry/config.env` | InfluxDB-Ziel + **Write-Token** | `config.env.example` |
| `/etc/uhr-telemetry/devices.json` | **Geräte-Keys** → device_id | `devices.json.example` |
| nginx `location = /telemetry` | im Port-80-Block von `/etc/nginx/sites-available/default` | — |

## Setup (Kurzfassung)

1. **InfluxDB Write-only-Token** (nur Bucket `uhren`, nicht der Operator-Token):
   ```
   influx auth create --org mikotec \
     --write-bucket "$(influx bucket list --org mikotec --name uhren --hide-headers | awk '{print $1}')" \
     --description "Heartbeat-Write Uhren-Flotte (nginx)"
   ```
2. **Dienst** (auf update-server): System-User `uhrtel` anlegen, `server.py` nach
   `/opt/uhr-telemetry/`, `config.env` + `devices.json` nach `/etc/uhr-telemetry/`
   (`chmod 640`, `chown root:uhrtel`), Unit installieren, `systemctl enable --now uhr-telemetry`.
3. **nginx**: `location = /telemetry { proxy_pass http://127.0.0.1:8099; ... }` in den
   Port-80-Block (vor der 301-Redirect-Regel), dann `nginx -t && systemctl reload nginx`.
   `/updates/`, der 8443-Block und der 443-vhost bleiben unberührt.

## Key-Verwaltung

- **Neue Uhr:** Zeile `"<key>": "<device_id>"` in `/etc/uhr-telemetry/devices.json`.
  Key erzeugen: `openssl rand -hex 24`.
- **Widerruf:** Zeile entfernen.
- Beides **ohne Neustart** – die Datei wird pro Request frisch gelesen.

## Sicherheit

- Write-only-Token (nur Bucket `uhren`, kein Read/Admin), einzeln widerrufbar.
- Pro-Gerät-Keys, einzeln widerrufbar, kein geteiltes Passwort.
- InfluxDB (8086) nur intern erreichbar (ufw), keine Portfreigabe.
- Dienst läuft als unprivilegierter User `uhrtel` mit systemd-Hardening.

## Offen

- **Firmware-Heartbeat** in der Uhr (HTTP-POST an `/telemetry`, heap-schonend).
- **Grafana**: InfluxDB-Datasource auf `192.168.10.25:8086` (eigener Read-Token) + Dashboard.
