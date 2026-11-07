#!/usr/bin/env python3
# Produktions-Start mit waitress. Bindet an LISTEN_ADDR/LISTEN_PORT aus der
# config.env (Default LAN-IP, nie 0.0.0.0).
from waitress import serve
from app import app, init_db, CFG

if __name__ == "__main__":
    init_db()
    host = CFG.get("LISTEN_ADDR", "127.0.0.1")
    port = int(CFG.get("LISTEN_PORT", "8080"))
    serve(app, host=host, port=port, threads=6, ident="uhr-fleet")
