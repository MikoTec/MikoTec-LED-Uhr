#!/usr/bin/env python3
# MikoTec LED-Uhr - Fleet-Web (Verwaltungs-Oberflaeche, lokal-only)
#
# Zeigt und verwaltet die Uhren-Flotte:
#   - Live-Heartbeats aus InfluxDB (Bucket "uhren", Measurement "heartbeat")
#   - Registry (SQLite) je device_id: Name, Standort, Notiz, Status
#   - rendert daraus die Allow-/Blocklist (devices.json) fuer den
#     Telemetrie-Dienst auf dem update-server (.20 holt sie per Sync-Timer).
#
# WICHTIG - LOKAL ONLY:
#   Bindet NUR an LISTEN_ADDR (Default die LAN-IP der VM), nie 0.0.0.0.
#   Niemals per FRITZ!Box nach aussen freigeben. ufw erlaubt den Web-Port
#   nur aus dem internen Subnetz (siehe install.sh).
#
# Secrets (InfluxDB-Read-Token, optionales Admin-Passwort, Pull-Token,
# Flask-Secret) stehen NICHT in dieser Datei, sondern in
# /etc/uhr-fleet/config.env.

import csv
import io
import json
import os
import re
import sqlite3
import time
import secrets as pysecrets
from functools import wraps
from datetime import datetime, timezone

import requests
from flask import (Flask, request, redirect, url_for, render_template,
                   session, abort, Response, flash, g)

# ----------------------------------------------------------------------------
# Konfiguration
# ----------------------------------------------------------------------------
CONFIG_PATH = os.environ.get("UHR_FLEET_CONFIG", "/etc/uhr-fleet/config.env")


def load_config(path):
    cfg = {}
    try:
        with open(path) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#") and "=" in line:
                    k, v = line.split("=", 1)
                    cfg[k.strip()] = v.strip()
    except FileNotFoundError:
        pass
    return cfg


CFG = load_config(CONFIG_PATH)

INFLUX_URL = CFG.get("INFLUX_URL", "http://127.0.0.1:8086").rstrip("/")
INFLUX_ORG = CFG.get("INFLUX_ORG", "mikotec")
INFLUX_BUCKET = CFG.get("INFLUX_BUCKET", "uhren")
INFLUX_READ_TOKEN = CFG.get("INFLUX_READ_TOKEN", "")

DB_PATH = CFG.get("DB_PATH", "/var/lib/uhr-fleet/registry.db")
ADMIN_PASSWORD = CFG.get("ADMIN_PASSWORD", "")      # leer => kein Login
PULL_TOKEN = CFG.get("PULL_TOKEN", "")              # fuer /api/devices.json (.20-Sync)
SECRET_KEY = CFG.get("SECRET_KEY") or pysecrets.token_hex(32)

# Heartbeat alle 15 min -> online, wenn letzter Kontakt juenger als 20 min.
ONLINE_WINDOW_S = int(CFG.get("ONLINE_WINDOW_S", "1200"))
# Zeitfenster, in dem wir ueberhaupt nach Heartbeats suchen.
LOOKBACK = CFG.get("LOOKBACK", "30d")

ID_RE = re.compile(r'^[0-9A-Za-z_-]{1,32}$')

app = Flask(__name__)
app.config["SECRET_KEY"] = SECRET_KEY
app.config["SESSION_COOKIE_SAMESITE"] = "Lax"
app.config["ONLINE_WINDOW_S"] = ONLINE_WINDOW_S


# ----------------------------------------------------------------------------
# SQLite-Registry
# ----------------------------------------------------------------------------
def db():
    if "db" not in g:
        g.db = sqlite3.connect(DB_PATH)
        g.db.row_factory = sqlite3.Row
        g.db.execute("PRAGMA busy_timeout=4000")
    return g.db


@app.teardown_appcontext
def close_db(exc):
    d = g.pop("db", None)
    if d is not None:
        d.close()


def init_db():
    con = sqlite3.connect(DB_PATH)
    con.execute("""CREATE TABLE IF NOT EXISTS devices(
        device_id TEXT PRIMARY KEY,
        name      TEXT NOT NULL DEFAULT '',
        location  TEXT NOT NULL DEFAULT '',
        notes     TEXT NOT NULL DEFAULT '',
        state     TEXT NOT NULL DEFAULT 'pending',
        created_at INTEGER NOT NULL,
        updated_at INTEGER NOT NULL
    )""")
    con.execute("""CREATE TABLE IF NOT EXISTS settings(
        key   TEXT PRIMARY KEY,
        value TEXT NOT NULL
    )""")
    con.execute("INSERT OR IGNORE INTO settings(key,value) VALUES('acl_mode','blacklist')")
    con.commit()
    con.close()


def get_setting(key, default=None):
    row = db().execute("SELECT value FROM settings WHERE key=?", (key,)).fetchone()
    return row["value"] if row else default


def set_setting(key, value):
    db().execute("INSERT INTO settings(key,value) VALUES(?,?) "
                 "ON CONFLICT(key) DO UPDATE SET value=excluded.value", (key, str(value)))
    db().commit()


def ensure_device(device_id):
    """Legt einen pending-Eintrag an, falls die device_id noch unbekannt ist."""
    now = int(time.time())
    db().execute(
        "INSERT OR IGNORE INTO devices(device_id,created_at,updated_at) VALUES(?,?,?)",
        (device_id, now, now))
    db().commit()


def registry_map():
    rows = db().execute("SELECT * FROM devices").fetchall()
    return {r["device_id"]: dict(r) for r in rows}


# ----------------------------------------------------------------------------
# InfluxDB-Abfrage (Flux -> annotierte CSV -> Zeilen)
# ----------------------------------------------------------------------------
def flux(query):
    if not INFLUX_READ_TOKEN:
        raise RuntimeError("Kein INFLUX_READ_TOKEN in der config.env gesetzt.")
    r = requests.post(
        "%s/api/v2/query?org=%s" % (INFLUX_URL, INFLUX_ORG),
        data=query.encode("utf-8"),
        headers={"Authorization": "Token " + INFLUX_READ_TOKEN,
                 "Content-Type": "application/vnd.flux",
                 "Accept": "application/csv"},
        timeout=8)
    r.raise_for_status()
    return parse_annotated_csv(r.text)


def parse_annotated_csv(text):
    """InfluxDB liefert annotierte CSV (mehrere Tabellen, #-Kommentarzeilen)."""
    rows = []
    header = None
    reader = csv.reader(io.StringIO(text))
    for rec in reader:
        if not rec:
            header = None
            continue
        if rec[0].startswith("#"):          # Annotationszeile (#datatype, #group, ...)
            continue
        if header is None:
            header = rec
            continue
        rows.append(dict(zip(header, rec)))
    return rows


def latest_per_device():
    """
    Neuester Heartbeat je device_id. Ein Heartbeat schreibt die drei Felder
    (uptime, free_heap, crash_info) mit identischem _time und identischen Tags.
    Wir holen last() je Serie und fassen pro device_id den juengsten Zeitpunkt
    zusammen.
    """
    q = ('from(bucket:"%s")\n'
         '  |> range(start:-%s)\n'
         '  |> filter(fn:(r)=> r._measurement=="heartbeat")\n'
         '  |> last()\n') % (INFLUX_BUCKET, LOOKBACK)
    try:
        rows = flux(q)
    except Exception as e:
        app.logger.warning("influx query failed: %s", e)
        return {}, str(e)

    devs = {}
    for r in rows:
        dev = r.get("device_id", "")
        if not dev:
            continue
        t = r.get("_time", "")
        d = devs.setdefault(dev, {
            "device_id": dev, "fw_version": "", "reset_reason": "",
            "uptime": None, "free_heap": None, "crash_info": "", "_time": ""})
        # pro Feldserie die juengste behalten
        if t >= d["_time"]:
            # Bei neuerem Zeitpunkt die Tags aktualisieren
            if t > d["_time"]:
                d["fw_version"] = r.get("fw_version", d["fw_version"])
                d["reset_reason"] = r.get("reset_reason", d["reset_reason"])
                d["_time"] = t
            field = r.get("_field", "")
            val = r.get("_value", "")
            if field == "uptime":
                d["uptime"] = to_int(val)
            elif field == "free_heap":
                d["free_heap"] = to_int(val)
            elif field == "crash_info":
                d["crash_info"] = val
    return devs, None


def history(device_id, limit=50):
    q = ('from(bucket:"%s")\n'
         '  |> range(start:-%s)\n'
         '  |> filter(fn:(r)=> r._measurement=="heartbeat" and r.device_id=="%s")\n'
         '  |> pivot(rowKey:["_time"], columnKey:["_field"], valueColumn:"_value")\n'
         '  |> sort(columns:["_time"], desc:true)\n'
         '  |> limit(n:%d)\n') % (INFLUX_BUCKET, LOOKBACK, device_id, limit)
    try:
        rows = flux(q)
    except Exception as e:
        app.logger.warning("history query failed: %s", e)
        return []
    out = []
    for r in rows:
        out.append({
            "time": r.get("_time", ""),
            "fw_version": r.get("fw_version", ""),
            "reset_reason": r.get("reset_reason", ""),
            "uptime": to_int(r.get("uptime", "")),
            "free_heap": to_int(r.get("free_heap", "")),
            "crash_info": r.get("crash_info", ""),
        })
    return out


def to_int(v):
    try:
        return int(float(v))
    except (TypeError, ValueError):
        return None


# ----------------------------------------------------------------------------
# Darstellungs-Helfer
# ----------------------------------------------------------------------------
def parse_rfc3339(s):
    if not s:
        return None
    s = s.replace("Z", "+00:00")
    try:
        return datetime.fromisoformat(s)
    except ValueError:
        # z.B. Nanosekunden abschneiden
        m = re.match(r'(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(\.\d+)?(.*)', s)
        if not m:
            return None
        base = m.group(1) + (m.group(3) or "+00:00")
        try:
            return datetime.fromisoformat(base)
        except ValueError:
            return None


@app.template_filter("humasize")
def humasize(n):
    if n is None:
        return "–"
    n = float(n)
    for unit in ("B", "KB", "MB"):
        if n < 1024 or unit == "MB":
            return ("%.1f %s" % (n, unit)).replace(".0 ", " ")
        n /= 1024
    return "%.1f MB" % n


@app.template_filter("humauptime")
def humauptime(sec):
    if sec is None:
        return "–"
    sec = int(sec)
    d, rem = divmod(sec, 86400)
    h, rem = divmod(rem, 3600)
    m, _ = divmod(rem, 60)
    parts = []
    if d:
        parts.append("%dd" % d)
    if h or d:
        parts.append("%dh" % h)
    parts.append("%dm" % m)
    return " ".join(parts)


@app.template_filter("humasince")
def humasince(iso):
    dt = parse_rfc3339(iso)
    if not dt:
        return "nie"
    delta = datetime.now(timezone.utc) - dt
    s = int(delta.total_seconds())
    if s < 60:
        return "vor %d s" % s
    if s < 3600:
        return "vor %d min" % (s // 60)
    if s < 86400:
        return "vor %d h" % (s // 3600)
    return "vor %d d" % (s // 86400)


def is_online(iso):
    dt = parse_rfc3339(iso)
    if not dt:
        return False
    return (datetime.now(timezone.utc) - dt).total_seconds() < ONLINE_WINDOW_S


# ----------------------------------------------------------------------------
# Login (optional)
# ----------------------------------------------------------------------------
def login_required(f):
    @wraps(f)
    def wrap(*a, **kw):
        if ADMIN_PASSWORD and not session.get("auth"):
            return redirect(url_for("login", next=request.path))
        return f(*a, **kw)
    return wrap


def csrf_token():
    if "_csrf" not in session:
        session["_csrf"] = pysecrets.token_hex(16)
    return session["_csrf"]


app.jinja_env.globals["csrf_token"] = csrf_token
app.jinja_env.globals["login_enabled"] = bool(ADMIN_PASSWORD)


def check_csrf():
    tok = request.form.get("_csrf", "")
    if not tok or tok != session.get("_csrf"):
        abort(400, "CSRF")


@app.route("/login", methods=["GET", "POST"])
def login():
    if not ADMIN_PASSWORD:
        return redirect(url_for("index"))
    if request.method == "POST":
        if pysecrets.compare_digest(request.form.get("password", ""), ADMIN_PASSWORD):
            session["auth"] = True
            nxt = request.args.get("next") or url_for("index")
            return redirect(nxt)
        flash("Falsches Passwort.")
    return render_template("login.html")


@app.route("/logout", methods=["POST"])
def logout():
    check_csrf()
    session.pop("auth", None)
    return redirect(url_for("login"))


# ----------------------------------------------------------------------------
# Views
# ----------------------------------------------------------------------------
def build_rows():
    """Registry + Live-Daten zu einer Tabelle zusammenfuehren."""
    live, err = latest_per_device()
    # unbekannte, aber sendende Uhren automatisch als pending anlegen
    reg = registry_map()
    new = False
    for dev in live:
        if dev not in reg:
            ensure_device(dev)
            new = True
    if new:
        reg = registry_map()

    rows = []
    for dev, meta in reg.items():
        l = live.get(dev, {})
        rows.append({
            "device_id": dev,
            "name": meta["name"],
            "location": meta["location"],
            "notes": meta["notes"],
            "state": meta["state"],
            "fw_version": l.get("fw_version", ""),
            "uptime": l.get("uptime"),
            "free_heap": l.get("free_heap"),
            "reset_reason": l.get("reset_reason", ""),
            "crash_info": l.get("crash_info", ""),
            "last_seen": l.get("_time", ""),
            "online": is_online(l.get("_time", "")),
            "has_live": bool(l),
        })
    # Sortierung: online zuerst, dann nach Name/ID
    rows.sort(key=lambda r: (not r["online"], (r["name"] or r["device_id"]).lower()))
    return rows, err


@app.route("/")
@login_required
def index():
    rows, err = build_rows()
    mode = get_setting("acl_mode", "blacklist")
    stats = {
        "total": len(rows),
        "online": sum(1 for r in rows if r["online"]),
        "blocked": sum(1 for r in rows if r["state"] == "blocked"),
        "pending": sum(1 for r in rows if r["state"] == "pending"),
        "crashes": sum(1 for r in rows if r["crash_info"]),
    }
    return render_template("index.html", rows=rows, err=err, mode=mode, stats=stats)


@app.route("/api/status")
@login_required
def api_status():
    """JSON fuer das Auto-Refresh im Dashboard."""
    rows, err = build_rows()
    out = []
    for r in rows:
        out.append({
            "device_id": r["device_id"],
            "online": r["online"],
            "fw_version": r["fw_version"],
            "uptime": humauptime(r["uptime"]),
            "free_heap": humasize(r["free_heap"]),
            "reset_reason": r["reset_reason"],
            "crash_info": r["crash_info"],
            "last_seen": humasince(r["last_seen"]),
            "state": r["state"],
        })
    return {"rows": out, "err": err}


@app.route("/device/<device_id>")
@login_required
def device(device_id):
    if not ID_RE.match(device_id):
        abort(404)
    reg = registry_map().get(device_id)
    if not reg:
        # evtl. sendet die Uhr, ist aber noch nicht in der Registry
        live, _ = latest_per_device()
        if device_id in live:
            ensure_device(device_id)
            reg = registry_map().get(device_id)
        else:
            abort(404)
    hist = history(device_id)
    latest = hist[0] if hist else {}
    online = is_online(latest.get("time", "")) if latest else False
    return render_template("device.html", d=reg, hist=hist, latest=latest, online=online)


@app.route("/device/<device_id>/save", methods=["POST"])
@login_required
def device_save(device_id):
    check_csrf()
    if not ID_RE.match(device_id):
        abort(404)
    ensure_device(device_id)
    name = request.form.get("name", "").strip()[:80]
    location = request.form.get("location", "").strip()[:80]
    notes = request.form.get("notes", "").strip()[:500]
    db().execute("UPDATE devices SET name=?,location=?,notes=?,updated_at=? WHERE device_id=?",
                 (name, location, notes, int(time.time()), device_id))
    db().commit()
    flash("Gespeichert.")
    return redirect(request.form.get("next") or url_for("device", device_id=device_id))


@app.route("/device/<device_id>/state", methods=["POST"])
@login_required
def device_state(device_id):
    check_csrf()
    if not ID_RE.match(device_id):
        abort(404)
    new = request.form.get("state", "")
    if new not in ("approved", "blocked", "pending"):
        abort(400)
    ensure_device(device_id)
    db().execute("UPDATE devices SET state=?,updated_at=? WHERE device_id=?",
                 (new, int(time.time()), device_id))
    db().commit()
    flash({"approved": "Uhr freigegeben.", "blocked": "Uhr gesperrt.",
           "pending": "Status zurueckgesetzt."}[new])
    return redirect(request.form.get("next") or url_for("index"))


@app.route("/device/<device_id>/delete", methods=["POST"])
@login_required
def device_delete(device_id):
    check_csrf()
    if not ID_RE.match(device_id):
        abort(404)
    db().execute("DELETE FROM devices WHERE device_id=?", (device_id,))
    db().commit()
    flash("Eintrag entfernt (kommt wieder, wenn die Uhr erneut sendet).")
    return redirect(url_for("index"))


@app.route("/settings/mode", methods=["POST"])
@login_required
def settings_mode():
    check_csrf()
    mode = request.form.get("mode", "")
    if mode not in ("blacklist", "whitelist"):
        abort(400)
    set_setting("acl_mode", mode)
    flash("Modus gesetzt: %s" % ("Whitelist (nur freigegebene)" if mode == "whitelist"
                                 else "Blacklist (alle ausser gesperrte)"))
    return redirect(url_for("index"))


# ----------------------------------------------------------------------------
# devices.json fuer den Telemetrie-Dienst (.20 holt das per Sync-Timer)
# ----------------------------------------------------------------------------
def render_devices_json():
    mode = get_setting("acl_mode", "blacklist")
    rows = db().execute("SELECT device_id,state FROM devices").fetchall()
    block = [r["device_id"] for r in rows if r["state"] == "blocked"]
    if mode == "whitelist":
        allow = [r["device_id"] for r in rows if r["state"] == "approved"]
    else:
        allow = []
    return {
        "_hinweis": "Automatisch erzeugt von uhr-fleet (.25). Nicht von Hand editieren.",
        "_mode": mode,
        "_generated": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "allow": sorted(allow),
        "block": sorted(block),
    }


@app.route("/api/devices.json")
def api_devices_json():
    # Durch Pull-Token geschuetzt (der .20-Sync schickt ihn im Header).
    if PULL_TOKEN:
        sent = request.headers.get("X-Pull-Token", "")
        if not pysecrets.compare_digest(sent, PULL_TOKEN):
            abort(401)
    return Response(json.dumps(render_devices_json(), indent=2) + "\n",
                    mimetype="application/json")


# ----------------------------------------------------------------------------
if __name__ == "__main__":
    init_db()
    app.run(host=CFG.get("LISTEN_ADDR", "127.0.0.1"),
            port=int(CFG.get("LISTEN_PORT", "8080")))
