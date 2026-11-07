#!/bin/bash
# Installer fuer die Fleet-Web-App auf der DB-VM (.25). Idempotent.
# Aufruf als root aus dem Repo heraus:  sudo ./install.sh
set -euo pipefail

[ "$(id -u)" -eq 0 ] || { echo "Bitte als root ausfuehren (sudo ./install.sh)"; exit 1; }
SRC="$(cd "$(dirname "$0")" && pwd)"

APP=/opt/uhr-fleet
ETC=/etc/uhr-fleet
LIB=/var/lib/uhr-fleet
USER=uhrfleet

echo "==> Pakete (apt, kein pip)"
apt-get update -qq
apt-get install -y python3-flask python3-waitress python3-requests >/dev/null

echo "==> System-User $USER"
id -u "$USER" >/dev/null 2>&1 || useradd --system --no-create-home --shell /usr/sbin/nologin "$USER"

echo "==> Verzeichnisse"
mkdir -p "$APP/templates" "$APP/static" "$ETC" "$LIB"

echo "==> App-Dateien nach $APP"
install -m 644 "$SRC/app.py" "$SRC/run.py" "$SRC/wsgi.py" "$APP/"
cp -r "$SRC/templates/." "$APP/templates/"
[ -d "$SRC/static" ] && cp -r "$SRC/static/." "$APP/static/" 2>/dev/null || true
chown -R root:root "$APP"
chown -R "$USER:$USER" "$LIB"
chmod 750 "$LIB"

echo "==> Konfiguration $ETC/config.env"
if [ ! -f "$ETC/config.env" ]; then
    LANIP="$(ip -o -4 addr show scope global | awk '{print $4}' | cut -d/ -f1 | head -1)"
    SECRET="$(openssl rand -hex 32)"
    PULL="$(openssl rand -hex 24)"
    cp "$SRC/config.env.example" "$ETC/config.env"
    sed -i "s|^LISTEN_ADDR=.*|LISTEN_ADDR=${LANIP:-127.0.0.1}|" "$ETC/config.env"
    sed -i "s|^SECRET_KEY=.*|SECRET_KEY=$SECRET|" "$ETC/config.env"
    sed -i "s|^PULL_TOKEN=.*|PULL_TOKEN=$PULL|" "$ETC/config.env"
    echo "    Neue config.env erzeugt (LISTEN_ADDR=${LANIP:-127.0.0.1}, SECRET_KEY + PULL_TOKEN gesetzt)."
    NEWCFG=1
else
    echo "    Bestehende config.env unveraendert gelassen."
    NEWCFG=0
fi
chown root:"$USER" "$ETC/config.env"
chmod 640 "$ETC/config.env"

echo "==> systemd-Dienst"
install -m 644 "$SRC/uhr-fleet.service" /etc/systemd/system/uhr-fleet.service
systemctl daemon-reload
systemctl enable uhr-fleet >/dev/null 2>&1 || true

echo "==> Firewall (ufw): Web-Port nur aus dem lokalen Subnetz"
PORT="$(awk -F= '/^LISTEN_PORT=/{print $2}' "$ETC/config.env")"
LANIP="$(awk -F= '/^LISTEN_ADDR=/{print $2}' "$ETC/config.env")"
SUBNET="$(echo "$LANIP" | awk -F. '{print $1"."$2"."$3".0/24"}')"
if command -v ufw >/dev/null 2>&1; then
    ufw allow from "$SUBNET" to any port "$PORT" proto tcp comment "uhr-fleet web (LAN only)" >/dev/null || true
    echo "    erlaubt: $SUBNET -> Port $PORT/tcp"
fi

echo "==> Dienst starten"
systemctl restart uhr-fleet
sleep 1
systemctl --no-pager --lines=0 status uhr-fleet || true

echo
echo "============================================================"
echo " Fleet-Web installiert."
echo " Oberflaeche:  http://$LANIP:$PORT   (nur im LAN erreichbar)"
echo
echo " NOCH ZU TUN in $ETC/config.env, dann 'systemctl restart uhr-fleet':"
echo "  1) INFLUX_READ_TOKEN=  -> in InfluxDB einen READ-Token fuer Bucket"
echo "     'uhren' anlegen (Data -> API Tokens -> Custom -> Read: uhren)."
echo "  2) ADMIN_PASSWORD=     -> Passwort fuer die Weboberflaeche setzen (empfohlen)."
echo
if [ "$NEWCFG" = "1" ]; then
  echo " PULL_TOKEN (fuer den .20-Sync) lautet:"
  awk -F= '/^PULL_TOKEN=/{print "      "$2}' "$ETC/config.env"
fi
echo "============================================================"
