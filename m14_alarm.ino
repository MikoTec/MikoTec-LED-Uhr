// m14_alarm.ino
//
// Wecker-/Alarm-System (wie ein Handy-Wecker):
// - bis zu MAX_ALARMS Alarme, jeder einzeln aktivierbar
// - pro Alarm: Uhrzeit, Wochentage (Bitmask), Effekt, Dauer (Minuten)
// - Effekte: 0 = kompletter strandtest (alle im Wechsel),
//            1 = colorWipe, 2 = theaterChase, 3 = Regenbogen
// - laeuft die eingestellte Dauer, danach zurueck zur normalen Uhr
// Persistenz: EEPROM ab Adresse 370 (6 Bytes je Alarm).
// Effekte sind non-blocking (kein delay/WDT), die Uhr bleibt responsiv.

// EEPROM-Basisadresse der Alarme (nach rebootHour=368, rebootMinute=369)
#define ALARM_EEPROM_BASE 370

// ---- Laden / Speichern ----

void loadAlarms() {
  EEPROM.begin(512);
  for (int i = 0; i < MAX_ALARMS; i++) {
    int a = ALARM_EEPROM_BASE + i * 6;
    alarms[i].enabled  = EEPROM.read(a);
    alarms[i].hour     = EEPROM.read(a + 1);
    alarms[i].minute   = EEPROM.read(a + 2);
    alarms[i].days     = EEPROM.read(a + 3);
    alarms[i].effect   = EEPROM.read(a + 4);
    alarms[i].duration = EEPROM.read(a + 5);
    // Sanity-Check (unbeschriebenes EEPROM liefert 255)
    if (alarms[i].enabled > 1)  alarms[i].enabled = 0;
    if (alarms[i].hour > 23)    alarms[i].hour = 7;
    if (alarms[i].minute > 59)  alarms[i].minute = 0;
    if (alarms[i].days > 127)   alarms[i].days = 0;
    if (alarms[i].effect > 3)   alarms[i].effect = 0;
    if (alarms[i].duration < 1 || alarms[i].duration > 30) alarms[i].duration = 2;
  }
}

void saveAlarms() {
  EEPROM.begin(512);
  for (int i = 0; i < MAX_ALARMS; i++) {
    int a = ALARM_EEPROM_BASE + i * 6;
    EEPROM.write(a,     alarms[i].enabled);
    EEPROM.write(a + 1, alarms[i].hour);
    EEPROM.write(a + 2, alarms[i].minute);
    EEPROM.write(a + 3, alarms[i].days);
    EEPROM.write(a + 4, alarms[i].effect);
    EEPROM.write(a + 5, alarms[i].duration);
  }
  EEPROM.commit();
}

// ---- Alarm-Pruefung ----

// Liefert das heutige Tages-Bit (bit0=Montag ... bit6=Sonntag) aus TimeLib weekday()
// TimeLib: weekday() 1=Sonntag, 2=Montag, ... 7=Samstag
static uint8_t todayDayBit() {
  int wd = weekday();
  int idx = (wd == 1) ? 6 : (wd - 2); // So->6, Mo->0, Di->1, ...
  return (uint8_t)(1 << idx);
}

// Prueft einmal pro Minute, ob ein Alarm jetzt starten soll.
void checkAlarms() {
  static int lastMinute = -1;
  if (minute() == lastMinute) return; // nur einmal pro Minute
  lastMinute = minute();

  if (activeAlarm >= 0) return; // es laeuft bereits ein Alarm

  uint8_t dayBit = todayDayBit();
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (!alarms[i].enabled) continue;
    if (!(alarms[i].days & dayBit)) continue;
    if (alarms[i].hour == hour() && alarms[i].minute == minute()) {
      activeAlarm = i;
      alarmStartMs = millis();
      alarmEffectPhase = 0;
      alarmEffectStep = 0;
      logTS(); dualOut.print("[ALARM] gestartet: #"); dualOut.print(i);
      dualOut.print(" Effekt "); dualOut.print(alarms[i].effect);
      dualOut.print(" Dauer "); dualOut.print(alarms[i].duration); dualOut.println(" min");
      break;
    }
  }
}

// true, solange ein Alarm laeuft (Dauer noch nicht abgelaufen)
bool alarmRunning() {
  if (activeAlarm < 0) return false;
  unsigned long durMs = (unsigned long)alarms[activeAlarm].duration * 60000UL;
  if (millis() - alarmStartMs > durMs) {
    logTS(); dualOut.println("[ALARM] beendet -> zurueck zur Uhr");
    activeAlarm = -1;
    return false;
  }
  return true;
}

// ---- Effekt-Engine (non-blocking) ----
// Einzelschritte der strandtest-Effekte. phase waehlt den Effekt-Abschnitt.

void alarmEffectShow(uint8_t effect) {
  if (!clockleds) return;

  static unsigned long lastStep = 0;
  static uint16_t stepDelay = 40;
  const uint8_t B = 120; // moderate Helligkeit (schont Netzteil)

  if (millis() - lastStep < stepDelay) return;
  lastStep = millis();

  // Effekt-Phasen je nach gewaehltem Effekt eingrenzen:
  //  strandtest(0): Phasen 0..6 (alles)
  //  colorWipe(1):  Phasen 0..2
  //  theaterChase(2): Phase 3
  //  rainbow(3):    Phasen 4..5
  uint8_t phMin, phMax;
  switch (effect) {
    case 1: phMin = 0; phMax = 2; break;
    case 2: phMin = 3; phMax = 3; break;
    case 3: phMin = 4; phMax = 5; break;
    default: phMin = 0; phMax = 6; break; // 0 = kompletter strandtest
  }
  if (alarmEffectPhase < phMin || alarmEffectPhase > phMax) {
    alarmEffectPhase = phMin;
    alarmEffectStep = 0;
  }

  uint16_t &step = alarmEffectStep;
  uint8_t  &phase = alarmEffectPhase;

  switch (phase) {
    // colorWipe rot/gruen/blau
    case 0:
    case 1:
    case 2: {
      RgbColor col = (phase == 0) ? RgbColor(B, 0, 0)
                   : (phase == 1) ? RgbColor(0, B, 0)
                                  : RgbColor(0, 0, B);
      clockleds->SetPixelColor(step, col);
      clockleds->Show();
      stepDelay = 15;
      if (++step >= (uint16_t)pixelCount) {
        step = 0;
        phase = (phase >= phMax) ? phMin : phase + 1;
      }
      break;
    }
    // theaterChase weiss
    case 3: {
      uint8_t q = step % 3;
      for (int i = 0; i < pixelCount; i++)
        clockleds->SetPixelColor(i, ((i % 3) == q) ? RgbColor(B, B, B) : RgbColor(0, 0, 0));
      clockleds->Show();
      stepDelay = 60;
      if (++step >= 30) {
        step = 0;
        phase = (phase >= phMax) ? phMin : phase + 1;
      }
      break;
    }
    // rainbow (ganzes Band eine durchlaufende Farbe)
    case 4: {
      float hue = (float)(step % 256) / 256.0f;
      for (int i = 0; i < pixelCount; i++)
        clockleds->SetPixelColor(i, RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f)));
      clockleds->Show();
      stepDelay = 12;
      if (++step >= 256) {
        step = 0;
        phase = (phase >= phMax) ? phMin : phase + 1;
      }
      break;
    }
    // rainbowCycle (Regenbogen ueber das Band verteilt, rotiert)
    case 5: {
      for (int i = 0; i < pixelCount; i++) {
        float hue = (float)(((i * 256 / pixelCount) + step) % 256) / 256.0f;
        clockleds->SetPixelColor(i, RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f)));
      }
      clockleds->Show();
      stepDelay = 12;
      if (++step >= 512) {
        step = 0;
        phase = (phase >= phMax) ? phMin : phase + 1;
      }
      break;
    }
    // theaterChaseRainbow
    case 6: {
      uint8_t q = step % 3;
      float hue = (float)(step % 256) / 256.0f;
      for (int i = 0; i < pixelCount; i++)
        clockleds->SetPixelColor(i, ((i % 3) == q)
                    ? RgbColor(HsbColor(hue, 1.0f, (float)B / 255.0f))
                    : RgbColor(0, 0, 0));
      clockleds->Show();
      stepDelay = 60;
      if (++step >= 90) {
        step = 0;
        phase = phMin; // zurueck zum Anfang -> Dauerschleife
      }
      break;
    }
    default:
      phase = phMin;
      step = 0;
      break;
  }
}

// Zeigt den Effekt des gerade aktiven Alarms
void alarmShow() {
  if (activeAlarm < 0) return;
  alarmEffectShow(alarms[activeAlarm].effect);
}
