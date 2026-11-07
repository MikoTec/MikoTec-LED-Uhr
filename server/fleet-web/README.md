# Fleet-Web – Verwaltungs-Oberfläche der Uhren-Flotte (lokal)

Schlanke, **rein lokale** Weboberfläche zum **Ansehen und Verwalten** der
MikoTec-LED-Uhren. Läuft auf der DB-VM **`UHR-DB-SRV` (192.168.10.25)** neben
InfluxDB. Grafana bleibt außen vor (nur für grafische Übersichten).

> **Lokal only.** Der Dienst bindet ausschließlich an die LAN-IP, `ufw` lässt den
> Web-Port nur aus dem internen Subnetz zu, und es gibt **keine** FRITZ!Box-Freigabe.
> Niemals nach außen öffnen.

## Was die Seite kann

- **Live-Status je Uhr** aus InfluxDB (Bucket `uhren`, Measurement `heartbeat`):
  Firmware-Version, Uptime, freier Heap, letzter Reset-Grund, Crash-Info,
  „zuletzt gesehen", Online/Offline.
- **Stammdaten bearbeiten**: Name, Standort, Notiz je Uhr.
- **Zugriff verwalten (Allow-/Blocklist der Chip-IDs)**: Uhr **freigeben**,
  **sperren**, **entsperren**. Neue Uhren tauchen automatisch als „neu" auf,
  sobald sie den ersten Heartbeat senden.
- **Modus**: *Blacklist* (alle außer Gesperrte) oder *Whitelist* (nur Freigegebene).

## Identität & Zugriff (wie im Telemetrie-Aufbau)

Es gibt **keine** Pro-Uhr-Secret-Keys. Die **ESP-Chip-ID** (`device_id`) ist die
unveränderliche Identität je Uhr; die Authentifizierung ist ein **geteilter
Key** (für alle Uhren gleich). Die scharfe, **einzeln widerrufbare** Kontrolle
ist die **Allow-/Blocklist der Chip-IDs** – genau die verwaltet diese Seite.

## Architektur / Datenfluss

```
Fleet-Web (.25, Flask+waitress)          devices.json-Sync (.20, systemd-Timer)
  ├─ liest Live-Heartbeats ───────────────► InfluxDB (.25:8086, lokal)
  ├─ Registry (SQLite, .25)  ← Name/Status je device_id
  └─ /api/devices.json  ◄──── alle 60 s ─── curl X-Pull-Token ──┐
                                                                 ▼
                              schreibt /etc/uhr-telemetry/devices.json (.20)
                                                                 ▼
                              server.py (Telemetrie) liest sie pro Request neu
```

Die Allow-/Blocklist-**Quelle** ist jetzt die Registry auf .25; der bestehende
Telemetrie-Dienst auf .20 wird **nicht** verändert – ein 60-s-Timer zieht die
gerenderte `devices.json` von der App. Fällt die App aus, behält .20 die letzte
Liste.

## Dateien

| Pfad (Server) | Inhalt | im Repo |
|---|---|---|
| `/opt/uhr-fleet/` | App (`app.py`, `run.py`, `templates/`) | `app.py`, … |
| `/etc/uhr-fleet/config.env` | InfluxDB-**Read-Token**, Admin-PW, **Pull-Token** | `config.env.example` |
| `/var/lib/uhr-fleet/registry.db` | SQLite-Registry (Namen, Status) | — |
| `/etc/systemd/system/uhr-fleet.service` | systemd-Unit | `uhr-fleet.service` |
| **auf .20:** `/opt/uhr-devices-sync/` + Timer | holt `devices.json` | `sync/` |

## Installation

**Auf .25** (im Repo-Checkout):
```
sudo apt install -y git
sudo ./install.sh
```
Danach in `/etc/uhr-fleet/config.env`:
1. `INFLUX_READ_TOKEN=` – in InfluxDB einen **Read-Token** für Bucket `uhren` anlegen.
2. `ADMIN_PASSWORD=` – Passwort für die Oberfläche (empfohlen).
3. `systemctl restart uhr-fleet`

**Auf .20** (im Repo-Checkout, Ordner `sync/`):
```
sudo ./install-sync.sh
```
Danach in `/etc/uhr-devices-sync/config.env` den **PULL_TOKEN** eintragen
(identisch zu dem aus der Fleet-Web-config.env), dann
`systemctl start uhr-devices-sync.service`.

## Sicherheit

- Bindet nur an die LAN-IP, `ufw` erlaubt den Port nur aus `…/24`, keine Freigabe.
- Optionales Admin-Passwort (Session-Login, CSRF-Schutz auf allen Formularen).
- `/api/devices.json` ist durch den **Pull-Token** geschützt.
- InfluxDB-**Read-only**-Token (nur Bucket `uhren`).
- Dienst läuft als unprivilegierter User `uhrfleet` mit systemd-Hardening.
