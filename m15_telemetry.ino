// m15_telemetry.ino
// Schlanker HTTP-Heartbeat an den zentralen Telemetrie-Endpunkt (Variante B).
//
// device_id = unveraenderliche ESP-Chip-ID (ESP.getChipId(), eindeutig pro Chip)
//             -> stabile Zuordnung der Messwerte in der DB, nicht konfigurierbar.
// Auth       = geteilter Schluessel aus secrets.h (sagt dem Server nur
//             "echte MikoTec-FW"). secrets.h ist per .gitignore nicht im Repo.
// Transport  = HTTP Port 80 (kein TLS -> kein BearSSL-Heap), kurzlebiger Request.
// Rhythmus   = erster Heartbeat ~30 s nach Boot, danach alle 15 min. Non-blocking.

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
    http.end();
  } // Scope-Ende: Client hier freigegeben
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
