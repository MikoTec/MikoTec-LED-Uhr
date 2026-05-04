// m10_nightcheck.ino

void nightCheck() {
  static int lastLoggedSleepH = -1;
  static int lastLoggedSleepM = -1;
  static int lastLoggedWakeH = -1;
  static int lastLoggedWakeM = -1;

  int sleepH = sleep;
  int sleepM = sleepmin;
  int wakeH = wake;
  int wakeM = wakemin;

  if (autoSleep == 1) {
    // Tag des Jahres berechnen
    int m = month();
    int d = day();
    int doy = d;
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int y = year();
    if (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) daysInMonth[2] = 29;
    for (int i = 1; i < m; i++) doy += daysInMonth[i];

    float tz = timezone + DSTtime;
    int sunriseH, sunriseM, sunsetH, sunsetM;
    int srMin, ssMin;
    getSunTimes(doy, latitude, longitude, tz, srMin, ssMin);
    sunriseH = srMin / 60; sunriseM = srMin % 60;
    sunsetH  = ssMin / 60; sunsetM  = ssMin % 60;

    // Globale sleep/wake Variablen aktualisieren
    sleep = sunsetH;
    sleepmin = sunsetM;
    wake = sunriseH;
    wakemin = sunriseM;

    sleepH = sleep;
    sleepM = sleepmin;
    wakeH = wake;
    wakeM = wakemin;

    // Nur loggen wenn sich die Werte geaendert haben
    if (sunsetH != lastLoggedSleepH || sunsetM != lastLoggedSleepM ||
        sunriseH != lastLoggedWakeH || sunriseM != lastLoggedWakeM) {
      lastLoggedSleepH = sunsetH;
      lastLoggedSleepM = sunsetM;
      lastLoggedWakeH = sunriseH;
      lastLoggedWakeM = sunriseM;
      logTS(); dualOut.print("Auto-Schlaf Sonnenuntergang: ");
      dualOut.print(sunsetH); dualOut.print(":");
      if (sunsetM < 10) dualOut.print("0");
      dualOut.println(sunsetM);
      logTS(); dualOut.print("Auto-Wach Sonnenaufgang: ");
      dualOut.print(sunriseH); dualOut.print(":");
      if (sunriseM < 10) dualOut.print("0");
      dualOut.println(sunriseM);
    }
  }

  int nowMin = hour() * 60 + minute();
  int sleepMin = sleepH * 60 + sleepM;
  int wakeMin = wakeH * 60 + wakeM;

  // clockmode nur aendern wenn wir nicht im dawnmode sind
  // dawnmode wird vom Dawn-Timer selbst beendet
  if (clockmode != dawnmode) {
    if (sleepMin > wakeMin) {
      if (nowMin >= sleepMin || nowMin < wakeMin) {
        clockmode = night;
      } else {
        clockmode = normal;
      }
    } else {
      if (nowMin >= sleepMin && nowMin < wakeMin) {
        clockmode = night;
      } else {
        clockmode = normal;
      }
    }
  }
}
