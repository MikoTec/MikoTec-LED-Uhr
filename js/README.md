# JavaScript-Dateien (ab v2.2.0.0)

Dieser Ordner enthält die JavaScript-Dateien für das LittleFS-Dateisystem des ESP8266.

## Geplante Dateien
- `clock.js` — Hauptlogik der Uhranzeige (bisher clockjs.h / /clock.js Endpunkt)
- `menu.js` — Menü-Overlay Logik (bisher Teil von menu.h)
- `settings.js` — Einstellungsseite Logik (bisher inline in settings.h)
- `support.js` — Log-Anzeige und Intervall-Steuerung (bisher inline in support.h)
- `timezone.js` — Standortsuche und Tab-Logik (bisher inline in timezone.h)

## Konzept
JavaScript wird direkt aus dem Flash geliefert, kein Inline-JS mehr in den HTML-Dateien.
Dynamische Werte kommen per fetch() von /getstate und anderen API-Endpunkten.
