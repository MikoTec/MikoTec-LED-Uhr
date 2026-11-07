// m15_telemetry.ino
// Schlanker HTTP-Heartbeat an den zentralen Telemetrie-Endpunkt (Variante B)
// UND Geraete-Sperre (Kill-Switch), gesteuert ueber den HTTP-Status der Antwort.
//
// device_id = unveraenderliche ESP-Chip-ID (ESP.getChipId(), eindeutig pro Chip).
// Auth       = geteilter Schluessel aus secrets.h (Header X-Device-Key).
// Transport  = HTTP Port 80 (kein TLS -> kein BearSSL-Heap), kurzlebiger Request.
// Rhythmus   = erster Heartbeat ~30 s nach Boot, danach alle 15 min. Non-blocking.
//
// Kill-Switch (FAIL-OPEN):
//   HTTP 403 -> sperren (deviceLocked = true)
//   HTTP 204 -> entsperren (deviceLocked = false)
//   alles andere (401, Timeout/-1, 5xx, DNS-Fehler) -> Zustand UNVERAENDERT.
// deviceLocked wird persistent in EEPROM-Byte DEVICELOCK_EEPROM_ADDR gehalten.

#include "secrets.h"

static const char*        TELEMETRY_URL         = "http://yzdlcru01ktmqlzy.myfritz.net/telemetry";
static const unsigned long TELEMETRY_INTERVAL    = 15UL * 60UL * 1000UL; // 15 min
static const unsigned long TELEMETRY_FIRST_DELAY = 30000UL;              // 30 s nach Boot
static unsigned long telemetryLast = 0;
static bool telemetryPrimed = false;

String telemetryDeviceId() {
  return String(ESP.getChipId(), HEX); // unveraenderlich, eindeutig pro Chip
}

static String telemetryJsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\r", " ");
  s.replace("\n", " ");
  return s;
}

// Setzt den Sperrzustand und persistiert ihn (nur bei echter Aenderung -> schont Flash).
void setDeviceLocked(bool locked) {
  if (locked == deviceLocked) return;
  deviceLocked = locked;
  EEPROM.begin(512);
  EEPROM.write(DEVICELOCK_EEPROM_ADDR, locked ? 1 : 0);
  EEPROM.commit();
  logTS(); dualOut.print("[LOCK] Zustand geaendert: ");
  dualOut.println(locked ? "GESPERRT" : "entsperrt");
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  String dev = telemetryDeviceId();
  String rr  = ESP.getResetReason();
  String ci  = "";
  if (rr.indexOf("xception") >= 0) ci = ESP.getResetInfo(); // crash_info nur bei Exception

  String body = "{";
  body += "\"device_id\":\"" + dev + "\",";
  body += "\"fw_version\":\"" + String(firmware_version) + "\",";
  body += "\"uptime\":" + String(millis() / 1000UL) + ",";
  body += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  body += "\"reset_reason\":\"" + telemetryJsonEscape(rr) + "\",";
  body += "\"crash_info\":\"" + telemetryJsonEscape(ci) + "\"";
  body += "}";

  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, TELEMETRY_URL);
    http.setTimeout(8000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Key", TELEMETRY_SHARED_KEY);
    int code = http.POST(body);
    logTS(); dualOut.print("[TELEMETRY] POST "); dualOut.print(code);
    dualOut.print(" dev="); dualOut.println(dev);
    // Kill-Switch, FAIL-OPEN: nur echtes 403 sperrt, nur 204 entsperrt.
    // Jeder andere Code (401, -1/Timeout, 5xx, DNS) laesst den Zustand unveraendert
    // -> eine Uhr wird NIE wegen Server-Ausfall gesperrt.
    if (code == 403) setDeviceLocked(true);
    else if (code == 204) setDeviceLocked(false);
    http.end();
  } // Scope-Ende: Client hier freigegeben
}

// Frueher Status-Check beim Boot (max. Heap), aktualisiert deviceLocked schnell.
void telemetryBootCheck() {
  sendHeartbeat();
}

void sendHeartbeatIfDue() {
  static bool idLogged = false;
  if (!idLogged) {
    logTS(); dualOut.print("[TELEMETRY] device_id: "); dualOut.println(telemetryDeviceId());
    idLogged = true;
  }
  unsigned long now = millis();
  if (!telemetryPrimed) {
    if (now >= TELEMETRY_FIRST_DELAY) {
      sendHeartbeat();
      telemetryLast = now;
      telemetryPrimed = true;
    }
    return;
  }
  if (now - telemetryLast >= TELEMETRY_INTERVAL) {
    sendHeartbeat();
    telemetryLast = now;
  }
}

// Sperr-Anzeige: alle LEDs ~1 Hz rot blinken (500 ms an / 500 ms aus).
// Gedaempftes Rot (150) schont das Netzteil, bleibt aber deutlich sichtbar.
void lockBlink() {
  if (!clockleds) return;
  static unsigned long lastBlink = 0;
  static bool on = false;
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    on = !on;
    RgbColor col = on ? RgbColor(150, 0, 0) : RgbColor(0, 0, 0);
    for (int i = 0; i < pixelCount; i++) clockleds->SetPixelColor(i, col);
    clockleds->Show();
  }
}
