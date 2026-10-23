// m14_blinktest.ino
//
// EINMALIGER OTA-TEST (24.10.2026):
// Am 24.10.2026 zwischen 6:45 und 7:15 Uhr blinkt die Uhr bunt (Regenbogen).
// Dient als sichtbarer Nachweis, dass das automatische OTA-Update
// (Firmware + LittleFS) ueber den naechtlichen Neustart funktioniert hat.
// Ausserhalb dieses einen Zeitfensters passiert nichts.
// Dieses Modul wird in der finalen Version wieder entfernt.

bool blinkTestActive() {
  // Nur am 24.10.2026
  if (year() != 2026 || month() != 10 || day() != 24) return false;
  // Nur zwischen 6:45 und 7:15 Uhr
  int nowMin = hour() * 60 + minute();
  return (nowMin >= (6 * 60 + 45) && nowMin <= (7 * 60 + 15));
}

void blinkTestShow() {
  if (!clockleds) return;
  static uint16_t offset = 0;
  static unsigned long lastBlink = 0;
  // Zusaetzlich an/aus blinken (alle 400 ms umschalten) fuer deutliche Sichtbarkeit
  static bool on = true;
  if (millis() - lastBlink > 400) {
    on = !on;
    lastBlink = millis();
  }
  for (int i = 0; i < pixelCount; i++) {
    if (on) {
      float hue = ((float)((i * 256 / pixelCount + offset) & 0xFF)) / 255.0f;
      clockleds->SetPixelColor(i, RgbColor(HsbColor(hue, 1.0f, 0.5f)));
    } else {
      clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
    }
  }
  clockleds->Show();
  offset += 3; // Regenbogen langsam rotieren
}
