#!/bin/bash
# MikoTec LED Uhr - Update Check Script
# Laedt alle neuen .bin Dateien von GitHub herunter
# Schreibt nur einen Log wenn sich etwas geaendert hat

UPDATE_DIR="/var/www/html/updates"
LOG_DIR="$UPDATE_DIR/logs"
GITHUB_API="https://api.github.com/repos/MikoTec/MikoTec-LED-Uhr/contents/updates"
GITHUB_RAW="https://raw.githubusercontent.com/MikoTec/MikoTec-LED-Uhr/main/updates"

# Log-Verzeichnis sicherstellen
mkdir -p "$LOG_DIR"

# Puffer fuer Log-Eintraege - wird nur geschrieben wenn CHANGED=1
LOG_BUFFER=""
CHANGED=0

buf() {
    LOG_BUFFER="$LOG_BUFFER\n[$(date '+%d.%m.%Y %H:%M:%S')] $1"
}

# GitHub API abfragen
API_RESPONSE=$(curl -s -w "\n%{http_code}" "$GITHUB_API")
HTTP_CODE=$(echo "$API_RESPONSE" | tail -1)
API_BODY=$(echo "$API_RESPONSE" | sed '$d')

if [ "$HTTP_CODE" != "200" ]; then
    # API-Fehler immer loggen
    CHANGED=1
    buf "========================================="
    buf "Update-Check gestartet"
    buf "========================================="
    buf "FEHLER: GitHub API nicht erreichbar (HTTP $HTTP_CODE)"
    buf "Update-Check beendet mit Fehler"
fi

if [ "$CHANGED" -eq 1 ]; then
    LOG_FILE="$LOG_DIR/updatecheck_$(date '+%Y%m%d_%H%M%S').log"
    echo -e "$LOG_BUFFER" >> "$LOG_FILE"
    exit 1
fi

# --- Firmware .bin Dateien ---
BIN_LIST=$(echo "$API_BODY" | grep -oP '"name": "MikoTec-LED-Uhr_v[^"]*\.bin"' | grep -oP 'MikoTec-LED-Uhr_v[^"]*\.bin')

for BIN_FILE in $BIN_LIST; do
    if [ ! -f "$UPDATE_DIR/$BIN_FILE" ]; then
        if [ "$CHANGED" -eq 0 ]; then
            buf "========================================="
            buf "Update-Check gestartet"
            buf "========================================="
        fi
        CHANGED=1
        buf "Neue Firmware-Datei: $BIN_FILE"

        DL_RESPONSE=$(curl -s -L -w "%{http_code}" -o "$UPDATE_DIR/$BIN_FILE" "$GITHUB_RAW/$BIN_FILE")
        DL_SIZE=$(stat -c%s "$UPDATE_DIR/$BIN_FILE" 2>/dev/null || echo "0")

        if [ "$DL_RESPONSE" = "200" ] && [ "$DL_SIZE" -gt 1000 ]; then
            buf "OK: $BIN_FILE heruntergeladen ($DL_SIZE Bytes)"
        else
            buf "FEHLER: Download $BIN_FILE fehlgeschlagen (HTTP $DL_RESPONSE, $DL_SIZE Bytes)"
            rm -f "$UPDATE_DIR/$BIN_FILE"
        fi
    fi
done

# --- LittleFS .bin Dateien ---
LFS_LIST=$(echo "$API_BODY" | grep -oP '"name": "littlefs[^"]*\.bin"' | grep -oP 'littlefs[^"]*\.bin')

for LFS_FILE in $LFS_LIST; do
    if [ ! -f "$UPDATE_DIR/$LFS_FILE" ]; then
        if [ "$CHANGED" -eq 0 ]; then
            buf "========================================="
            buf "Update-Check gestartet"
            buf "========================================="
        fi
        CHANGED=1
        buf "Neue LittleFS-Datei: $LFS_FILE"

        DL_RESPONSE=$(curl -s -L -w "%{http_code}" -o "$UPDATE_DIR/$LFS_FILE" "$GITHUB_RAW/$LFS_FILE")
        DL_SIZE=$(stat -c%s "$UPDATE_DIR/$LFS_FILE" 2>/dev/null || echo "0")

        if [ "$DL_RESPONSE" = "200" ] && [ "$DL_SIZE" -gt 1000 ]; then
            buf "OK: $LFS_FILE heruntergeladen ($DL_SIZE Bytes)"
        else
            buf "FEHLER: Download $LFS_FILE fehlgeschlagen (HTTP $DL_RESPONSE, $DL_SIZE Bytes)"
            rm -f "$UPDATE_DIR/$LFS_FILE"
        fi
    fi
done

# --- version.json pruefen ---
REMOTE_VER=$(curl -s -w "\n%{http_code}" "$GITHUB_RAW/version.json")
VER_HTTP=$(echo "$REMOTE_VER" | tail -1)
VER_BODY=$(echo "$REMOTE_VER" | sed '$d')

if [ "$VER_HTTP" = "200" ] && [ -n "$VER_BODY" ]; then
    LOCAL_VER=""
    if [ -f "$UPDATE_DIR/version.json" ]; then
        LOCAL_VER=$(cat "$UPDATE_DIR/version.json")
    fi

    if [ "$VER_BODY" != "$LOCAL_VER" ]; then
        if [ "$CHANGED" -eq 0 ]; then
            buf "========================================="
            buf "Update-Check gestartet"
            buf "========================================="
        fi
        CHANGED=1

        REMOTE_VERSION=$(echo "$VER_BODY" | grep -oP '(?<="version": ")[^"]*')
        REMOTE_LFS_VERSION=$(echo "$VER_BODY" | grep -oP '(?<="littlefs_version": ")[^"]*')
        LOCAL_VERSION="(keine)"
        if [ -n "$LOCAL_VER" ]; then
            LOCAL_VERSION=$(echo "$LOCAL_VER" | grep -oP '(?<="version": ")[^"]*')
        fi

        echo "$VER_BODY" > "$UPDATE_DIR/version.json"
        buf "version.json AKTUALISIERT: $LOCAL_VERSION -> $REMOTE_VERSION (LittleFS: $REMOTE_LFS_VERSION)"
    fi
else
    if [ "$CHANGED" -eq 0 ]; then
        buf "========================================="
        buf "Update-Check gestartet"
        buf "========================================="
    fi
    CHANGED=1
    buf "FEHLER: version.json nicht ladbar (HTTP $VER_HTTP)"
fi

# --- Log schreiben wenn etwas geaendert ---
if [ "$CHANGED" -eq 1 ]; then
    DISK_FREE=$(df -h "$UPDATE_DIR" | tail -1 | awk '{print $4}')
    buf "Freier Speicherplatz: $DISK_FREE"
    buf "Update-Check beendet"
    buf "========================================="

    LOG_FILE="$LOG_DIR/updatecheck_$(date '+%Y%m%d_%H%M%S').log"
    echo -e "$LOG_BUFFER" >> "$LOG_FILE"

    # Alte Logs aufraumen (aelter als 14 Tage)
    find "$LOG_DIR/" -name "updatecheck_*.log" -mtime +14 -delete 2>/dev/null
fi
