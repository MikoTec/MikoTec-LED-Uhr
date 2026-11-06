#!/usr/bin/env python3
# MikoTec LED-Uhr - Telemetrie-Endpunkt (Server-Seite, Variante B)
#
# Nimmt den HTTP-Heartbeat einer Uhr an und schreibt die Werte ins
# InfluxDB-Line-Protocol.
#
# Identitaet:  device_id = ESP-Chip-ID, kommt vom Client (unveraenderlich,
#              eindeutig pro Chip). Wird gegen [0-9A-Za-z_-]{1,32} validiert.
# Auth:        geteilter Schluessel (SHARED_KEY) im Header X-Device-Key -
#              sagt nur "echte MikoTec-FW".
# Zugriff:     Allow-/Blocklist der ESP-IDs in devices.json (pro Request neu
#              gelesen -> ohne Neustart aenderbar):
#                allow leer  -> alle ausser block (Blacklist-Modus)
#                allow gefuellt -> nur diese IDs (Whitelist-Modus)
#
# Secrets (SHARED_KEY, InfluxDB-Write-Token) stehen NICHT in dieser Datei,
# sondern in /etc/uhr-telemetry/config.env.
import json, sys, time, re, urllib.request
from http.server import BaseHTTPRequestHandler, HTTPServer

CFG = {}
with open("/etc/uhr-telemetry/config.env") as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1); CFG[k] = v

INFLUX = CFG["INFLUX_URL"].rstrip("/")
ORG, BUCKET, TOKEN = CFG["INFLUX_ORG"], CFG["INFLUX_BUCKET"], CFG["INFLUX_TOKEN"]
SHARED_KEY = CFG["SHARED_KEY"]
PORT = int(CFG.get("LISTEN_PORT", "8099"))
DEVICES = "/etc/uhr-telemetry/devices.json"
ID_RE = re.compile(r'^[0-9A-Za-z_-]{1,32}$')

def acl():
    try:
        with open(DEVICES) as f: return json.load(f)
    except Exception: return {"allow": [], "block": []}

def allowed(dev):
    a = acl(); allow = a.get("allow", []); block = a.get("block", [])
    if dev in block: return False
    if allow: return dev in allow
    return True

def tag(s):
    return str(s).replace("\\", "\\\\").replace(" ", "\\ ").replace(",", "\\,").replace("=", "\\=")
def strf(s):
    return str(s).replace("\\", "\\\\").replace('"', '\\"')

class H(BaseHTTPRequestHandler):
    def _r(self, code, msg=b""):
        self.send_response(code); self.send_header("Content-Length", str(len(msg))); self.end_headers()
        if msg: self.wfile.write(msg)
    def do_POST(self):
        if self.headers.get("X-Device-Key", "") != SHARED_KEY:
            sys.stderr.write("telemetry: bad shared key\n"); self._r(401, b"unauthorized\n"); return
        n = int(self.headers.get("Content-Length", "0") or 0)
        if n <= 0 or n > 2048: self._r(400, b"bad length\n"); return
        try: d = json.loads(self.rfile.read(n))
        except Exception: self._r(400, b"bad json\n"); return
        dev = str(d.get("device_id", ""))
        if not ID_RE.match(dev): self._r(400, b"bad device_id\n"); return
        if not allowed(dev):
            sys.stderr.write("telemetry: device not allowed: %s\n" % dev); self._r(403, b"device not allowed\n"); return
        try:
            uptime = int(d.get("uptime", 0)); free_heap = int(d.get("free_heap", 0))
        except Exception:
            self._r(400, b"bad numbers\n"); return
        fw = str(d.get("fw_version", "") or "unknown")
        rr = str(d.get("reset_reason", "") or "unknown")
        ci = str(d.get("crash_info", ""))
        lp = ('heartbeat,device_id=%s,fw_version=%s,reset_reason=%s '
              'uptime=%di,free_heap=%di,crash_info="%s" %d'
              % (tag(dev), tag(fw), tag(rr), uptime, free_heap, strf(ci), int(time.time())))
        req = urllib.request.Request(
            "%s/api/v2/write?org=%s&bucket=%s&precision=s" % (INFLUX, ORG, BUCKET),
            data=lp.encode(), method="POST",
            headers={"Authorization": "Token " + TOKEN, "Content-Type": "text/plain; charset=utf-8"})
        try:
            urllib.request.urlopen(req, timeout=5)
        except Exception as e:
            sys.stderr.write("telemetry: influx write failed: %s\n" % e); self._r(502, b"influx error\n"); return
        self._r(204)
    def do_GET(self): self._r(405, b"method not allowed\n")
    def log_message(self, *a): pass

if __name__ == "__main__":
    HTTPServer(("127.0.0.1", PORT), H).serve_forever()
