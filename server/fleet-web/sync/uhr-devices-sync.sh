#!/bin/sh
# Holt die Allow-/Blocklist von der Fleet-Web-App (.25) und schreibt sie
# atomar nach /etc/uhr-telemetry/devices.json. Laeuft per systemd-Timer auf
# dem update-server (.20). Schlaegt der Abruf fehl oder ist die Antwort kein
# gueltiges JSON, bleibt die bestehende devices.json unveraendert.
set -eu

CFG=/etc/uhr-devices-sync/config.env
# shellcheck disable=SC1090
. "$CFG"

: "${FLEET_URL:?FLEET_URL fehlt in $CFG}"
: "${PULL_TOKEN:?PULL_TOKEN fehlt in $CFG}"
DEST="${DEST:-/etc/uhr-telemetry/devices.json}"
OWNER="${OWNER:-root:uhrtel}"

TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT

# Abruf (max 8 s, keine stderr-Flut)
if ! curl -fsS --max-time 8 -H "X-Pull-Token: $PULL_TOKEN" "$FLEET_URL" -o "$TMP"; then
    echo "uhr-devices-sync: Abruf von $FLEET_URL fehlgeschlagen - behalte alte Liste" >&2
    exit 0
fi

# JSON validieren und Pflichtfelder pruefen
if ! python3 - "$TMP" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
assert isinstance(d.get("allow"), list) and isinstance(d.get("block"), list)
PY
then
    echo "uhr-devices-sync: Antwort ist kein gueltiges devices.json - behalte alte Liste" >&2
    exit 0
fi

# nur schreiben, wenn sich etwas geaendert hat (schont I/O / Logs)
if [ -f "$DEST" ] && cmp -s "$TMP" "$DEST"; then
    exit 0
fi

install -o "${OWNER%:*}" -g "${OWNER#*:}" -m 640 "$TMP" "$DEST"
echo "uhr-devices-sync: devices.json aktualisiert"
