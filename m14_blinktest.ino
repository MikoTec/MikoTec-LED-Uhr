// m14_blinktest.ino
//
// EINMALIGER OTA-TEST (24.10.2026):
// Am 24.10.2026 zwischen 6:45 und 7:15 Uhr laeuft ein "strandtest"-Durchlauf
// (klassisches NeoPixel-Demo): colorWipe, theaterChase, rainbow, rainbowCycle,
// theaterChaseRainbow - bunt wechselnde Farben und Effekte in Dauerschleife.
// Dient als sichtbarer Nachweis, dass das automatische OTA-Update
// (Firmware + LittleFS) ueber den naechtlichen Neustart funktioniert hat.
// Ausserhalb dieses einen Zeitfensters passiert nichts.
// Als non-blocking State-Machine ausgelegt, damit die Uhr responsiv bleibt
// (kein delay(), kein WDT-Reset). Dieses Modul wird in der finalen Version
// (2.3.0.50) wieder entfernt.

bool blinkTestActive() {
  // Nur am 24.10.2026
  if (year() != 2026 || month() != 10 || day() != 24) return false;
  // Nur zwischen 6:45 und 7:15 Uhr
  int nowMin = hour() * 60 + minute();
  return (nowMin >= (6 * 60 + 45) && nowMin <= (7 * 60 + 15));
}

void blinkTestShow() {
  if (!clockleds) return;

  static unsigned long lastStep = 0;
  static uint8_t  phase = 0;     // aktueller Effekt
  static uint16_t step = 0;      // Schritt im Effekt
  static uint16_t stepDelay = 40;

  if (millis() - lastStep < stepDelay) return;
  lastStep = millis();

  const uint8_t B = 120; // moderate Basis-Helligkeit (von 255), schont Netzteil

  switch (phase) {

    // --- colorWipe: Farbe laeuft Pixel fuer Pixel durch (rot, gruen, blau) ---
    case 0:
    case 1:
    case 2: {
      RgbColor col = (phase == 0) ? RgbColor(B, 0, 0)
                   : (phase == 1) ? RgbColor(0, B, 0)
                                  : RgbColor(0, 0, B);
      clockleds->SetPixelColor(step, col);
      clockleds->Show();
      stepDelay = 15;
      if (++step >= (uint16_t)pixelCount) { step = 0; phase++; }
      break;
    }

    // --- theaterChase: Lauflicht (jedes 3. Pixel an), weiss ---
    case 3: {
      uint8_t q = step % 3;
      for (int i = 0; i < pixelCount; i++) {
        clockleds->SetPixelColor(i, ((i % 3) == q) ? RgbColor(B, B, B) : RgbColor(0, 0, 0));
      }
      clockleds->Show();
      stepDelay = 60;
      if (++step >= 30) { step = 0; phase++; }
      break;
    }

    // --- rainbow: ganzes Band in einer Farbe, die den Regenbogen durchlaeuft ---
    case 4: {
      float hue = (float)(step % 256) / 256.0f;
      for (int i = 0; i < pixelCount; i++) {
        clockleds->SetPixelColor(i, RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f)));
      }
      clockleds->Show();
      stepDelay = 12;
      if (++step >= 256) { step = 0; phase++; }
      break;
    }

    // --- rainbowCycle: Regenbogen ueber das Band verteilt, rotiert ---
    case 5: {
      for (int i = 0; i < pixelCount; i++) {
        float hue = (float)(((i * 256 / pixelCount) + step) % 256) / 256.0f;
        clockleds->SetPixelColor(i, RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f)));
      }
      clockleds->Show();
      stepDelay = 12;
      if (++step >= 512) { step = 0; phase++; }
      break;
    }

    // --- theaterChaseRainbow: Lauflicht in wechselnden Regenbogenfarben ---
    case 6: {
      uint8_t q = step % 3;
      float hue = (float)(step % 256) / 256.0f;
      for (int i = 0; i < pixelCount; i++) {
        clockleds->SetPixelColor(i, ((i % 3) == q)
                    ? RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f))
                    : RgbColor(0, 0, 0));
      }
      clockleds->Show();
      stepDelay = 60;
      if (++step >= 90) { step = 0; phase = 0; } // zurueck zum Anfang -> Dauerschleife
      break;
    }

    default:
      phase = 0;
      step = 0;
      break;
  }
}
