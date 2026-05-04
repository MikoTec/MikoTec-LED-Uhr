// m03_sun.ino

void fetchSunriseSunset(float lat, float lng) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClient apiClient;
  HTTPClient http;
  char url[160];
  snprintf(url, sizeof(url),
    "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&daily=sunrise,sunset&timezone=auto&forecast_days=1",
    lat, lng);
  http.begin(apiClient, url);
  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    int si = body.indexOf("\"sunrise\":[\"");
    int ei = body.indexOf("\"sunset\":[\"");
    if (si > 0 && ei > 0) {
      int riseH = body.substring(si + 12 + 11, si + 12 + 13).toInt();
      int riseM = body.substring(si + 12 + 14, si + 12 + 16).toInt();
      int setH  = body.substring(ei + 11 + 11, ei + 11 + 13).toInt();
      int setM  = body.substring(ei + 11 + 14, ei + 11 + 16).toInt();
      int riseLocal = riseH * 60 + riseM;
      int setLocal  = setH  * 60 + setM;
      time_t t = NTPclient.getEpochTime();
      struct tm *ti = gmtime(&t);
      apiSunriseMinutes = riseLocal;
      apiSunsetMinutes  = setLocal;
      apiCacheDay       = ti->tm_mday;
      logTS(); dualOut.print("[SUN-API] Sonnenaufgang: ");
      dualOut.print(riseLocal / 60); dualOut.print(":"); if (riseLocal%60<10) dualOut.print("0"); dualOut.println(riseLocal % 60);
      logTS(); dualOut.print("[SUN-API] Sonnenuntergang: ");
      dualOut.print(setLocal / 60); dualOut.print(":"); if (setLocal%60<10) dualOut.print("0"); dualOut.println(setLocal % 60);
    } else {
      logTS(); dualOut.println("[SUN-API] Parse-Fehler");
    }
  } else {
    logTS(); dualOut.print("[SUN-API] Fehler: HTTP "); dualOut.println(code);
  }
  http.end();
}

void getSunTimes(int dayOfYear, float lat, float lng, float tz, int &sunriseMin, int &sunsetMin) {
  time_t t = NTPclient.getEpochTime();
  struct tm *ti = gmtime(&t);
  if (apiSunriseMinutes >= 0 && apiCacheDay == ti->tm_mday) {
    sunriseMin = apiSunriseMinutes;
    sunsetMin  = apiSunsetMinutes;
    return;
  }
  // Fallback: lokale Berechnung
  int srH, srM, ssH, ssM;
  calcSunriseSunset(dayOfYear, lat, lng, tz, srH, srM, ssH, ssM);
  sunriseMin = srH * 60 + srM;
  sunsetMin  = ssH * 60 + ssM;
}

void calcSunriseSunset(int dayOfYear, float lat, float lng, float tz, int &sunriseH, int &sunriseM, int &sunsetH, int &sunsetM) {
  // Vereinfachter Algorithmus basierend auf NOAA
  float radLat = lat * PI / 180.0;

  // Sonnen-Deklination (vereinfacht)
  float declination = -23.45 * cos(2.0 * PI * (dayOfYear + 10) / 365.0);
  float radDecl = declination * PI / 180.0;

  // Stundenwinkel
  float cosH = (sin(-0.8333 * PI / 180.0) - sin(radLat) * sin(radDecl)) / (cos(radLat) * cos(radDecl));

  // Polargebiete abfangen
  if (cosH > 1.0) { // Polarnacht
    sunriseH = 8; sunriseM = 0; sunsetH = 16; sunsetM = 0;
    return;
  }
  if (cosH < -1.0) { // Mitternachtssonne
    sunriseH = 3; sunriseM = 0; sunsetH = 23; sunsetM = 0;
    return;
  }

  float H = acos(cosH) * 180.0 / PI;

  // Zeitgleichung (vereinfacht)
  float B = 2.0 * PI * (dayOfYear - 81) / 365.0;
  float EoT = 9.87 * sin(2 * B) - 7.53 * cos(B) - 1.5 * sin(B);

  // Sonnenmittag in Minuten (UTC)
  float solarNoon = 720 - 4 * lng - EoT;

  // Sonnenaufgang und -untergang in Minuten (UTC)
  float sunriseUTC = solarNoon - H * 4;
  float sunsetUTC = solarNoon + H * 4;

  // Zeitzone anwenden
  float sunriseLocal = sunriseUTC + tz * 60;
  float sunsetLocal = sunsetUTC + tz * 60;

  // In Stunden und Minuten umrechnen
  sunriseH = ((int)sunriseLocal / 60) % 24;
  sunriseM = ((int)sunriseLocal) % 60;
  if (sunriseM < 0) sunriseM += 60;
  if (sunriseH < 0) sunriseH += 24;

  sunsetH = ((int)sunsetLocal / 60) % 24;
  sunsetM = ((int)sunsetLocal) % 60;
  if (sunsetM < 0) sunsetM += 60;
  if (sunsetH < 0) sunsetH += 24;
}
