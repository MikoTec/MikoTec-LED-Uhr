#!/usr/bin/env python3
# WSGI-Einstieg fuer waitress. Startet die App und stellt sicher, dass die
# SQLite-Registry existiert.
from app import app, init_db

init_db()
application = app
