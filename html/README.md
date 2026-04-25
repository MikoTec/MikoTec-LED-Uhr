# HTML-Dateien (ab v2.2.0.0)

Dieser Ordner enthält die statischen HTML-Seiten für das LittleFS-Dateisystem des ESP8266.

## Geplante Dateien
- `index.html` — Hauptseite (bisher root.h)
- `settings.html` — Einstellungen (bisher settings.h)
- `hilfe.html` — Hilfe (bisher hilfe.h)
- `support.html` — Support/Log (bisher support.h)
- `timezone.html` — Zeitzone (bisher timezone.h)
- `clearromsure.html` — Werksreset-Bestätigung (bisher clearromsure.h)
- `alarm.html` — Alarm (bisher alarm.h)
- `game.html` — Minispiel (bisher game.h)

## Konzept
Die Seiten werden direkt aus dem LittleFS-Flash geliefert (`server.serveStatic()`).
Dynamische Werte werden per JavaScript von `/getstate` und anderen API-Endpunkten geholt.
Kein HTML mehr im RAM — der Heap bleibt frei für den Betrieb.
