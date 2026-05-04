// m11_settings.ino

void handleSettings() {
  EEPROM.begin(512);
  
  // Settings-Formular verarbeiten wenn Parameter vorhanden
  if (server.hasArg("submit")) {
    logTS(); dualOut.print("Submit: ");
    dualOut.println(server.arg("submit"));
    logTS(); dualOut.print("Anzahl Args: ");
    dualOut.println(server.args());
    for (int i = 0; i < server.args(); i++) {
      dualOut.print("  "); dualOut.print(server.argName(i));
      dualOut.print("="); dualOut.println(server.arg(i));
    }
  }
  if (server.hasArg("hourmarks")) {
    String hourmarksstring = server.arg("hourmarks");
    hourmarks = hourmarksstring.toInt();
    EEPROM.write(181, hourmarks);
  }
  if (server.hasArg("sleeptype")) {
    String sleeptypestring = server.arg("sleeptype");
    sleeptype = sleeptypestring.toInt();
    EEPROM.write(228, sleeptype);
  }
  if (server.hasArg("nightbrightness")) {
    String nbstring = server.arg("nightbrightness");
    nightBrightness = nbstring.toInt();
    EEPROM.write(234, nightBrightness);
    logTS(); dualOut.print("nightBrightness gesetzt: ");
    dualOut.println(nightBrightness);
  }
  if (server.hasArg("sleep")) {
    String sleepstring = server.arg("sleep");
    sleep = sleepstring.substring(0, 2).toInt();
    sleepmin = sleepstring.substring(3).toInt();
    EEPROM.write(182, sleep);
    EEPROM.write(183, sleepmin);
  }
  if (server.hasArg("wake")) {
    String wakestring = server.arg("wake");
    wake = wakestring.substring(0, 2).toInt();
    wakemin = wakestring.substring(3).toInt();
    EEPROM.write(189, wake);
    EEPROM.write(190, wakemin);
    nightCheck();
  }
  if (server.hasArg("DSThidden")) {
    int oldDSTtime = DSTtime;
    DSTtime = server.hasArg("DST");
    EEPROM.write(192, DSTtime);
    NTPclient.setTimeOffset((timezone + DSTtime) * 3600);
    adjustTime((DSTtime - oldDSTtime) * 3600);
  }
  if (server.hasArg("showsecondshidden")) {
    showseconds = server.hasArg("showseconds");
    EEPROM.write(184, showseconds);
  }
  if (server.hasArg("showsunpointhidden")) {
    showSunPoint = server.hasArg("showsunpoint");
    EEPROM.write(235, showSunPoint);
  }
  if (server.hasArg("dawnbreakhidden")) {
    dawnbreak = server.hasArg("dawnbreak");
    EEPROM.write(229, dawnbreak);
  }
  if (server.hasArg("hemisphere")) {
    String hemiStr = server.arg("hemisphere");
    hemisphere = hemiStr.toInt();
    EEPROM.write(232, hemisphere);
  }
  if (server.hasArg("autosleep")) {
    String autoStr = server.arg("autosleep");
    autoSleep = autoStr.toInt();
    EEPROM.write(233, autoSleep);
  }
  if (server.hasArg("pixelCount")) {
    pixelCount = server.arg("pixelCount").toInt();
    ChangeNeoPixels(pixelCount, clockPin);
    EEPROM.write(230, pixelCount);
  }
  if (server.hasArg("maxbright")) {
    maxBrightness = server.arg("maxbright").toInt();
    brightness = maxBrightness;
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
  }
  if (server.hasArg("powerType")) {
    int pt = server.arg("powerType").toInt();
    maxBrightness = (pt == 1) ? 255 : 100;
    brightness = maxBrightness;
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
  }
  if (server.hasArg("timezone")) {
    timezonevalue = server.arg("timezone").toInt();
    EEPROM.write(179, timezonevalue);
    interpretTimeZone(timezonevalue);
  }
  if (server.hasArg("clockname")) {
    String tempclockname = server.arg("clockname");
    cleanASCII(tempclockname);
    if (tempclockname != clockname) {
      clockname = tempclockname;
      for (int i = 195; i < 228; i++) {
        EEPROM.write(i, 0);
      }
      for (unsigned int i = 0; i < clockname.length(); ++i) {
        EEPROM.write(195 + i, clockname[i]);
      }
    }
  }
  // Beta-Channel
  if (server.hasArg("betahidden")) {
    betaChannel = server.hasArg("betachannel");
    EEPROM.write(367, betaChannel ? 1 : 0);
  }
  EEPROM.commit();
  delay(100);

  logTS(); dualOut.println("Sending handleSettings");
  logTS(); dualOut.print("sleep: ");
  dualOut.println(timeToText(sleep, sleepmin));
  logTS(); dualOut.print("wake: ");
  dualOut.println(timeToText(wake, wakemin));
  
  // settings.html direkt aus LittleFS streamen - Werte kommen per JS fetch /getstate
  if (LittleFS.exists("/settings.html")) {
    logTS(); dualOut.println("Sending handleSettings (LittleFS)");
    File f = LittleFS.open("/settings.html", "r");
    server.streamFile(f, "text/html");
    f.close();
    EEPROM.commit();
    return;
  }

  String toSend = FPSTR(settings_html);
  toSend.replace("$timezonevalue", String(timezonevalue));
  for (int i = 0; i < 5; i++) {
    if (i == hourmarks) {
      toSend.replace("$hourmarks" + String(i), "selected");
    } else {
      toSend.replace("$hourmarks" + String(i), "");
    }
  }

  for (int i = 0; i < 5; i++) {
    if (i == sleeptype) {
      toSend.replace("$sleeptype" + String(i), "selected");
    } else {
      toSend.replace("$sleeptype" + String(i), "");
    }
  }

  for (int i = 0; i < 2; i++) {
    if (i == hemisphere) {
      toSend.replace("$hemisphere" + String(i), "selected");
    } else {
      toSend.replace("$hemisphere" + String(i), "");
    }
  }

  for (int i = 0; i < 2; i++) {
    if (i == autoSleep) {
      toSend.replace("$autosleep" + String(i), "selected");
    } else {
      toSend.replace("$autosleep" + String(i), "");
    }
  }

  if (pixelCount == 120 && maxBrightness == 255) { //check if we're in "normal" light clock mode
    toSend.replace("$original", "selected");
    toSend.replace("$mini", "");
    toSend.replace("$customtype", "");
    toSend.replace("$customvisible", "none");
  } else {

    if (pixelCount == 60 && maxBrightness == 100) { //check if we're in mini light clock mode
      toSend.replace("$original", "");
      toSend.replace("$mini", "selected");
      toSend.replace("$customtype", "");
      toSend.replace("$customvisible", "none");
    } else {
      toSend.replace("$original", "");
      toSend.replace("$mini", "");
      toSend.replace("$customtype", "selected");
      toSend.replace("$customvisible", "block");
    }
  }
  if (webMode != 2) {
    // don't send external links if we're local only
    toSend.replace("$externallinks", FPSTR(externallinks));
  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");

  }
  toSend.replace("$pixelCount", String(pixelCount));
  if (maxBrightness == 100) {
    toSend.replace("$maxbright100", "selected");
    toSend.replace("$maxbright255", "");
  } else {
    toSend.replace("$maxbright100", "");
    toSend.replace("$maxbright255", "selected");
  }
  String ischecked;
  showseconds ? ischecked = "checked" : ischecked = "";
  toSend.replace("$showseconds", ischecked);
  showSunPoint ? ischecked = "checked" : ischecked = "";
  toSend.replace("$showsunpoint", ischecked);
  dawnbreak ? ischecked = "checked" : ischecked = "";
  toSend.replace("$dawnbreak", ischecked);
  DSTtime ? ischecked = "checked" : ischecked = "";
  logTS(); dualOut.print("sleep: ");
  dualOut.println(timeToText(sleep, sleepmin));
  logTS(); dualOut.print("wake: ");
  dualOut.println(timeToText(wake, wakemin));
  toSend.replace("$DSTtime", ischecked);
  toSend.replace("$sleep", timeToText(sleep, sleepmin));
  toSend.replace("$wake", timeToText(wake, wakemin));
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$clockname", String(clockname));
  toSend.replace("$nightbrightness", String(int(nightBrightness)));
  toSend.replace("$firmware_version", firmware_version);

  server.send(200, "text/html", toSend);

}

void handleTimezone() {
  if (LittleFS.exists("/timezone.html")) {
    logTS(); dualOut.println("Sending handleTimezone (LittleFS)");
    File f = LittleFS.open("/timezone.html", "r");
    server.streamFile(f, "text/html");
    f.close();
    return;
  }
  String fontreplace;
  if (webMode == 1) {
    fontreplace = FPSTR(importfonts);
  } else {
    fontreplace = "";
  }
  String toSend = FPSTR(timezone_html);
  //toSend.replace("$css", css_file);
  //toSend.replace("$fonts", fontreplace);
  if (webMode != 2) {
    toSend.replace("$externallinks", FPSTR(externallinks));
  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");
  }
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$latitude", String(latitude));
  toSend.replace("$longitude", String(longitude));
  toSend.replace("$menu", FPSTR(menu_html));


  server.send(200, "text/html", toSend);

  logTS(); dualOut.println("Sending handleTimezone");
}


void webHandleClearRom() {
  String s;
  s = "<p>Clearing the EEPROM and reset to configure new wifi<p>";
  s += "</html>\r\n\r\n";
  logTS(); dualOut.println("Sending webHandleClearRom");
  server.send(200, "text/html", s);
  logTS(); dualOut.println("clearing eeprom");
  clearEEPROM();
  delay(10);
  logTS(); dualOut.println("Done, restarting!");
  ESP.reset();
}


void webHandleClearRomSure() {
  String toSend = FPSTR(clearromsure_html);
  //toSend.replace("$css", css_file);
  if (webMode != 2) {
    toSend.replace("$externallinks", FPSTR(externallinks));
  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");
  }
  toSend.replace("$menu", FPSTR(menu_html));
  logTS(); dualOut.println("Sending webHandleClearRomSure");
  server.send(200, "text/html", toSend);
}

//-------------------------text input conversion functions---------------------------------------------

void getRGB(String hexRGB, RgbColor &rgb) {
  hexRGB.toUpperCase();
  char c[7];
  hexRGB.toCharArray(c, 8);
  rgb.R = hexcolorToInt(c[1], c[2]); //red
  rgb.G = hexcolorToInt(c[3], c[4]); //green
  rgb.B = hexcolorToInt(c[5], c[6]); //blue
}

String rgbToText(RgbColor input) {
  //convert RGB values to #FFFFFF notation. Add in 0s where hexcode would be only a single digit.
  String out;
  out += "#";
  (String(input.R, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.R, HEX);
  (String(input.G, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.G, HEX);
  (String(input.B, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.B, HEX);

  return out;

}

String timeToText(int hours, int minutes) {
  String out;
  (String(hours, DEC)).length() == 1 ? out += "0" : out += "";
  out += String(hours, DEC);
  out += ":";
  (String(minutes, DEC)).length() == 1 ? out += "0" : out += "";
  out += String(minutes, DEC);
  return out;
}
