// m06_config.ino

void loadConfig() {
  logTS(); dualOut.println("reading settings from EEPROM");
  //Tries to read ssid and password from EEPROM
  EEPROM.begin(512);
  delay(10);

  for (int i = 0; i < 32; ++i)
  {
    char c = char(EEPROM.read(i));
    if (c == '\0' || c == (char)255) break;
    esid += c;
  }
  logTS(); dualOut.print("SSID: ");
  dualOut.println(esid);


  for (int i = 32; i < 96; ++i)
  {
    char c = char(EEPROM.read(i));
    if (c == '\0' || c == (char)255) break;
    epass += c;
  }
  logTS(); dualOut.print("PASS: ");
  dualOut.println(epass);

  clockname = "";
  for (int i = 195; i < 228; ++i)
  {
    clockname += char(EEPROM.read(i));
  }
  clockname = clockname.c_str();
  logTS(); dualOut.print("clockname: ");
  dualOut.println(clockname);


  loadFace(0);
  latitude = readLatLong(175);
  logTS(); dualOut.print("latitude: ");
  dualOut.println(latitude);
  longitude = readLatLong(177);
  logTS(); dualOut.print("longitude: ");
  dualOut.println(longitude);
  timezonevalue = EEPROM.read(179);
  logTS(); dualOut.print("timezonevalue: ");
  dualOut.println(timezonevalue);
  interpretTimeZone(timezonevalue);
  logTS(); dualOut.print("timezone: ");
  dualOut.println(timezone);
  randommode = EEPROM.read(180);
  logTS(); dualOut.print("randommode: ");
  dualOut.println(randommode);
  hourmarks = EEPROM.read(181);
  logTS(); dualOut.print("hourmarks: ");
  dualOut.println(hourmarks);
  sleep = EEPROM.read(182);
  logTS(); dualOut.print("sleep: ");
  dualOut.println(sleep);
  sleeptype = EEPROM.read(228);
  logTS(); dualOut.print("sleep: ");
  dualOut.println(sleep);
  sleepmin = EEPROM.read(183);
  logTS(); dualOut.print("sleepmin: ");
  dualOut.println(sleepmin);
  showseconds = EEPROM.read(184);
  logTS(); dualOut.print("showseconds: ");
  dualOut.println(showseconds);
  showSunPoint = EEPROM.read(235);
  if (showSunPoint > 1) showSunPoint = 0;
  logTS(); dualOut.print("showSunPoint: ");
  dualOut.println(showSunPoint);
  DSTauto = EEPROM.read(185);
  logTS(); dualOut.print("DSTauto: ");
  dualOut.println(DSTauto);
  webMode = EEPROM.read(186);
  logTS(); dualOut.print("webMode: ");
  dualOut.println(webMode);
  wake = EEPROM.read(189);
  logTS(); dualOut.print("wake: ");
  dualOut.println(wake);
  wakemin = EEPROM.read(190);
  logTS(); dualOut.print("wakemin: ");
  dualOut.println(wakemin);
  brightness = EEPROM.read(191);
  logTS(); dualOut.print("brightness: ");
  dualOut.println(brightness);
  DSTtime = EEPROM.read(192);
  logTS(); dualOut.print("DST (true/false): ");
  dualOut.println(DSTtime);
  hourofdeath = EEPROM.read(193);
  logTS(); dualOut.print("Hour of Death: ");
  dualOut.println(hourofdeath);
  minuteofdeath = EEPROM.read(194);
  logTS(); dualOut.print("minuteofdeath: ");
  dualOut.println(minuteofdeath);
  setTime(hourofdeath, minuteofdeath, 0, 0, 0, 0);
  dawnbreak = EEPROM.read(229);
  hemisphere = EEPROM.read(232);
  if (hemisphere > 1) hemisphere = 0;
  logTS(); dualOut.print("hemisphere: ");
  dualOut.println(hemisphere);
  autoSleep = EEPROM.read(233);
  if (autoSleep > 1) autoSleep = 0;
  logTS(); dualOut.print("autoSleep: ");
  dualOut.println(autoSleep);
  nightBrightness = EEPROM.read(234);
  if (nightBrightness > 100) nightBrightness = 10;
  logTS(); dualOut.print("nightBrightness: ");
  dualOut.println(nightBrightness);
  logTS(); dualOut.print("dawnbreak: ");
  dualOut.println(dawnbreak);
  pixelCount = EEPROM.read(230);
  logTS(); dualOut.print("pixelcount: ");
  dualOut.println(pixelCount);
  maxBrightness = EEPROM.read(231);
  logTS(); dualOut.print("maxBrightness: ");
  dualOut.println(maxBrightness);

  // MQTT Konfiguration laden
  loadMqttConfig();

  // Beta-Channel laden (unabhaengig von MQTT)
  betaChannel = EEPROM.read(367);
  if (betaChannel > 1) betaChannel = false;
  logTS(); dualOut.println("Beta-Channel: " + String(betaChannel ? "Ja" : "Nein"));
}

void writeInitalConfig() {
  logTS(); dualOut.println("can't find settings so writing defaults");
  EEPROM.begin(512);
  delay(10);
  writeLatLong(175, 51.17); //default to Solingen
  writeLatLong(177, 7.08);//default to Solingen
  EEPROM.write(179, 34);//timezone default CET (UTC+1) Solingen - case 34 = Amsterdam/Berlin/Bern
  EEPROM.write(180, 0);//default randommode off
  EEPROM.write(181, 0); //default hourmarks to off
  EEPROM.write(182, 22); //default to sleep at 22:00
  EEPROM.write(183, 0);
  EEPROM.write(184, 1); //default to showseconds to yes
  EEPROM.write(185, 1); //default DSTauto on fuer automatische Sommerzeit
  EEPROM.write(186, 0); //default webMode to setup mode off until user sets local wifi
  EEPROM.write(500, 196);//write magic byte to 500 so that system knows its set up.
  EEPROM.write(228, 1);//default sleeptype to 1 (dots)
  EEPROM.write(189, 7);//default wake 7 hours
  EEPROM.write(190, 0); //default to wake at 00 minutes
  EEPROM.write(191, 100); //default to full brightness on USB so as not to crash
  EEPROM.write(192, 0); //default no daylight savings
  EEPROM.write(193, 10); //default "hour of death" is 10am
  EEPROM.write(220, 1); //default dawnbreak to "on"
  EEPROM.write(230, 120); //default to normal size light clock
  EEPROM.write(231, 255); //default to mains power for max brightness
  EEPROM.write(232, 0); //default hemisphere to 0 (Nord)
  EEPROM.write(233, 0); //default autoSleep to 0 (manuell)
  EEPROM.write(234, 10); //default nightBrightness to 10%
  EEPROM.write(236, 0); //default MQTT disabled
  EEPROM.write(237, (1883 >> 8) & 0xFF); //default MQTT port high byte
  EEPROM.write(238, 1883 & 0xFF); //default MQTT port low byte
  for (int i = 239; i < 367; i++) { EEPROM.write(i, 0); } //clear MQTT broker/user/pass
  EEPROM.write(367, 0); //default beta channel off


  for (int i = 195; i < 228; i++) {//zero (instead of null) the values where clockname will be written.
    EEPROM.write(i, 0);
  }
  EEPROM.write(194, 10); //default "minute of death" is 10am
  for (int i = 0; i < clockname.length(); ++i) {
    EEPROM.write(195 + i, clockname[i]);
    dualOut.print(clockname[i]);
  }







  EEPROM.commit();
  delay(500);

  //face 1 defaults
  hourcolor = RgbColor(255, 255, 0);
  minutecolor = RgbColor(0, 57, 255);
  blendpoint = 70;
  saveFace(0);
  saveFace(1);
  //face 2 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(0, 0, 255);
  blendpoint = 60;
  saveFace(2);
  //face 3 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(255, 255, 0);
  blendpoint = 90;
  saveFace(3);
  //face 4 defaults (Werkseinstellung)
  hourcolor = RgbColor(255, 255, 0);
  minutecolor = RgbColor(0, 57, 255);
  blendpoint = 70;
  saveFace(4);

}
