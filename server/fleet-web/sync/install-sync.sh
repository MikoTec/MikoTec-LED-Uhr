#!/bin/bash
# Installer fuer den devices.json-Sync auf dem update-server (.20). Idempotent.
# Holt die Allow-/Blocklist alle 60 s von der Fleet-Web-App (.25).
# Aufruf als root aus dem Repo heraus:  sudo ./install-sync.sh
set -euo pipefail

[ "$(id -u)" -eq 0 ] || { echo "Bitte als root ausfuehren (sudo ./install-sync.sh)"; exit 1; }
SRC="$(cd "$(dirname "$0")" && pwd)"

APP=/opt/uhr-devices-sync
ETC=/etc/uhr-devices-sync

echo "==> curl + python3 vorhanden?"
command -v curl >/dev/null || apt-get install -y curl >/dev/null
command -v python3 >/dev/null || { echo "python3 fehlt"; exit 1; }

echo "==> Script nach $APP"
mkdir -p "$APP" "$ETC"
install -m 755 "$SRC/uhr-devices-sync.sh" "$APP/uhr-devices-sync.sh"

echo "==> Konfiguration $ETC/config.env"
if [ ! -f "$ETC/config.env" ]; then
    cp "$SRC/config.env.example" "$ETC/config.env"
    chmod 600 "$ETC/config.env"
    echo "    Neue config.env erzeugt - bitte PULL_TOKEN eintragen (siehe unten)."
    NEWCFG=1
else
    echo "    Bestehende config.env unveraendert gelassen."
    NEWCFG=0
fi

echo "==> systemd Timer + Service"
install -m 644 "$SRC/uhr-devices-sync.service" /etc/systemd/system/uhr-devices-sync.service
install -m 644 "$SRC/uhr-devices-sync.timer"   /etc/systemd/system/uhr-devices-sync.timer
systemctl daemon-reload
systemctl enable --now uhr-devices-sync.timer >/dev/null 2>&1 || true

echo
echo "============================================================"
if [ "$NEWCFG" = "1" ]; then
  echo " NOCH ZU TUN: In $ETC/config.env den PULL_TOKEN eintragen"
  echo " (identisch zu PULL_TOKEN aus der Fleet-Web config.env auf .25),"
  echo " dann:  systemctl start uhr-devices-sync.service"
else
  echo " Sync aktiv. Status:  systemctl list-timers uhr-devices-sync.timer"
fi
echo " Testlauf:  /opt/uhr-devices-sync/uhr-devices-sync.sh"
echo " Ergebnis:  cat /etc/uhr-telemetry/devices.json"
echo "============================================================"
