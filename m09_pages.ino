// m09_pages.ino

void handleNotFound() {
  logTS(); dualOut.println("Sending handleNotFound");
  logTS(); dualOut.print("\t\t\t\t URI Not Found: ");
  dualOut.println(server.uri());
  server.send ( 200, "text/plain", "URI Not Found" );
}

void handleCSS() {
  if (LittleFS.exists("/style.css")) {
    File f = LittleFS.open("/style.css", "r");
    server.sendHeader("Cache-Control", "max-age=3600");
    server.streamFile(f, "text/css"); f.close();
  } else { server.send(200, "text/css", css_file); }
  //WiFiClient client = server.client();
  //sendProgmem(client, css_file);
  logTS(); dualOut.println("Sending CSS");
}
void handlecolourjs() {
  server.send(200, "text/plain", FPSTR(colourjs));
  //WiFiClient client = server.client();
  //sendProgmem(client, colourjs);
  logTS(); dualOut.println("Sending colourjs");
}
void handlespectrumjs() {
  server.send(200, "text/plain", spectrumjs);
  //WiFiClient client = server.client();
  //sendProgmem(client, spectrumjs);
  logTS(); dualOut.println("Sending spectrumjs");
}
void handleclockjs() {
  if (LittleFS.exists("/clock.js")) {
    File f = LittleFS.open("/clock.js", "r");
    server.sendHeader("Cache-Control", "max-age=3600");
    server.streamFile(f, "text/javascript"); f.close();
  } else { server.send(200, "text/plain", FPSTR(clockjs)); }
  //WiFiClient client = server.client();
  //sendProgmem(client, clockjs);
  logTS(); dualOut.println("Sending clockjs");
}

void handlespectrumCSS() {

  server.send(200, "text/css", FPSTR(spectrumCSS));
  //WiFiClient client = server.client();
  //sendProgmem(client, spectrumCSS);
  logTS(); dualOut.println("Sending spectrumCSS");
}

void handleRoot() {
  float alarmHour;
  float alarmMin;
  float alarmSec;



  EEPROM.begin(512);

  RgbColor tempcolor;
  HslColor tempcolorHsl;

  if (server.hasArg("pixelCount")) {
    String pixelCountString = server.arg("pixelCount");  //get value from blend slider
    pixelCount = pixelCountString.toInt();//atoi(c);  //get value from html5 color element
    ChangeNeoPixels(pixelCount, clockPin);
    EEPROM.write(230, pixelCount);
  }

  if (server.hasArg("powerType")) {
    String powerTypeString = server.arg("powerType");  //get value from blend slider
    int powerType = powerTypeString.toInt();//atoi(c);  //get value from html5 color element
    if (powerType == 1) {
      maxBrightness = 255;
    } else {
      maxBrightness = 100;
      brightness = _min(maxBrightness, brightness);
    }
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
  }





  //Check for all the potential incoming arguments
  if (server.hasArg("alarmhour")) {
    String alarmHourString = server.arg("alarmhour");  //get value from blend slider
    alarmHour = alarmHourString.toInt();//atoi(c);  //get value from html5 color element
  }

  if (server.hasArg("alarmmin")) {
    String alarmMinString = server.arg("alarmmin");  //get value from blend slider
    alarmMin = alarmMinString.toInt();//atoi(c);  //get value from html5 color element
  }

  if (server.hasArg("alarmsec")) {
    String alarmSecString = server.arg("alarmsec");  //get value from blend slider
    alarmSec = alarmSecString.toInt();//atoi(c);  //turn value to number
    alarmprogress = 0;
    alarmtick.detach();
    logTS(); dualOut.println("alarm triggered");
    clockmode = alarm;

    alarmtick.attach((alarmHour * 3600 + alarmMin * 60 + alarmSec) / (float)pixelCount, alarmadvance);
  }

  if (server.hasArg("hourcolor")) {
    String hourrgbStr = server.arg("hourcolor");  //get value from html5 color element
    hourrgbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(hourrgbStr, hourcolor);
  }

  if (server.hasArg("minutecolor")) {
    String minutergbStr = server.arg("minutecolor");  //get value from html5 color element
    minutergbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(minutergbStr, minutecolor);               //convert RGB string to rgb ints
  }

  if (server.hasArg("alarmcolor")) {
    String minutergbStr = server.arg("alarmcolor");  //get value from html5 color element
    minutergbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(minutergbStr, alarmcolor);               //convert RGB string to rgb ints
  }
  if (server.hasArg("submit")) {


    String memoryarg = server.arg("submit");

    logTS(); dualOut.print("Submit: ");
    dualOut.println(memoryarg);

    // Deutsche Button-Werte: "Speichern V1", "Laden V1", etc.
    if (memoryarg.startsWith("Speichern V") || memoryarg.startsWith("Laden V")) {
      String location = memoryarg.substring(memoryarg.length() - 1);
      logTS(); dualOut.print("Location: ");
      dualOut.println(location);
      if (memoryarg.startsWith("Speichern")) {
        saveFace(location.toInt());
      } else {
        loadFace(location.toInt());
      }
    }
    // Fallback fuer alte englische Buttons
    else if (memoryarg.substring(5, 11) == "Scheme") {
      String saveload = memoryarg.substring(0, 4);
      String location = memoryarg.substring(12);
      if (saveload == "Save") {
        saveFace(location.toInt());
      } else {
        loadFace(location.toInt());
      }
    }
  }

  if (webMode == 2) {
    if (server.hasArg("hourcolorspectrum")) {
      String hourrgbStr = server.arg("hourcolorspectrum");  //get value from html5 color element
      hourrgbStr.replace("%23", "#"); //%23 = # in URI
      getRGB(hourrgbStr, hourcolor);
    }

    if (server.hasArg("minutecolorspectrum")) {
      String minutergbStr = server.arg("minutecolorspectrum");  //get value from html5 color element
      minutergbStr.replace("%23", "#"); //%23 = # in URI
      getRGB(minutergbStr, minutecolor);               //convert RGB string to rgb ints
    }

  }

  if (server.hasArg("blendpoint")) {
    String blendpointstring = server.arg("blendpoint");  //get value from blend slider
    blendpoint = blendpointstring.toInt();//atoi(c);  //get value from html5 color element

  }
  if (server.hasArg("brightness")) {
    String brightnessstring = server.arg("brightness");  //get value from blend slider
    brightness = _max((int)10, (int)brightnessstring.toInt());//atoi(c);  //get value from html5 color element
    logTS(); dualOut.print("brightness: ");
    dualOut.println(brightness);
    EEPROM.write(191, brightness);
  }

  if (server.hasArg("hourmarks")) {
    String hourmarksstring = server.arg("hourmarks");  //get value from blend slider
    hourmarks = hourmarksstring.toInt();//atoi(c);  //get value from html5 color element
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
    String sleepstring = server.arg("sleep");  //get value input
    sleep = sleepstring.substring(0, 2).toInt(); //atoi(c);  //get first section of string for hours
    sleepmin = sleepstring.substring(3).toInt();//atoi(c);  //get second section of string for minutes
    EEPROM.write(182, sleep);
    EEPROM.write(183, sleepmin);
  }
  if (server.hasArg("wake")) {
    String wakestring = server.arg("wake");  //get value from blend slider
    wake = wakestring.substring(0, 2).toInt(); //atoi(c);  //get value from html5 color element
    wakemin = wakestring.substring(3).toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(189, wake);
    EEPROM.write(190, wakemin);

    //update sleep/wake to current
    logTS(); dualOut.println("");
    logTS(); dualOut.print("time: ");
    dualOut.println(timeToText(hour(), minute()));
    logTS(); dualOut.print("sleep: ");
    dualOut.println(timeToText(sleep, sleepmin));
    logTS(); dualOut.print("wake: ");
    dualOut.println(timeToText(wake, wakemin));
    nightCheck();
  }
  if (server.hasArg("DSThidden")) {
    int oldDSTtime = DSTtime;
    DSTtime = server.hasArg("DST");
    EEPROM.write(192, DSTtime);
    NTPclient.setTimeOffset((timezone + DSTtime) * 3600); // Offset muss in Sekunden angegeben werden
    adjustTime((DSTtime - oldDSTtime) * 3600);
  }

  if (server.hasArg("timezone")) {
    int oldtimezone = timezone;
    String timezonestring = server.arg("timezone");
    timezonevalue = timezonestring.toInt();//atoi(c);
    interpretTimeZone(timezonevalue);
    NTPclient.setTimeOffset((timezone + DSTtime) * 3600); // Offset muss in Sekunden angegeben werden
    //setTime(NTPclient.getEpochTime());
    adjustTime((timezone - oldtimezone) * 3600);
    EEPROM.write(179, timezonevalue);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }



  if (server.hasArg("latitude")) {
    String latitudestring = server.arg("latitude");  //get value from blend slider
    latitude = latitudestring.toFloat();//atoi(c);  //get value from html5 color element
    writeLatLong(175, latitude);
  }
  if (server.hasArg("alarmoff")) {
    nightCheck();
    alarmtick.detach();
    logTS(); dualOut.print("alarmoff has triggered");
  }
  if (server.hasArg("longitude")) {
    String longitudestring = server.arg("longitude");  //get value from blend slider
    longitude = longitudestring.toFloat();//atoi(c);  //get value from html5 color element
    writeLatLong(177, longitude);
    DSTauto = 1;
    EEPROM.write(185, 1); //tell the system that DST is auto adjusting
    EEPROM.write(179, timezonevalue);  // Fix: timezonevalue (int) statt timezone (float)
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

  if (server.hasArg("clockname")) {
    String tempclockname = server.arg("clockname");
    cleanASCII(tempclockname);

    if (tempclockname != clockname) {
      clockname = tempclockname;

      dualOut.println(clockname);
      logTS(); dualOut.println("");
      logTS(); dualOut.println("clearing old clockname.");
      //clear the old clock name out
      for (int i = 195; i < 228; i++) {
        EEPROM.write(i, 0);
      }
      logTS(); dualOut.println("writing eeprom clockname.");

      int clockname_len = clockname.length() + 1;
      char clocknamechar[clockname_len];
      clockname.toCharArray(clocknamechar, clockname_len);
      if (!mdns.begin(clocknamechar)) {
        logTS(); dualOut.println("MDNS Fehler - wird beim Neustart aktiv.");
        // kein Endlosloop bei MDNS-Fehler
      } else {
        logTS(); dualOut.println("mDNS responder started");
      }
      for (int i = 0; i < clockname.length(); ++i) {
        EEPROM.write(195 + i, clockname[i]);
        dualOut.print(clockname[i]);
      }
      logTS(); dualOut.println("");
    }
  }
  //save the current colours in case of crash
  EEPROM.write(100, hourcolor.R);
  EEPROM.write(101, hourcolor.G);
  EEPROM.write(102, hourcolor.B);

  //write the minute color
  EEPROM.write(103, minutecolor.R);
  EEPROM.write(104, minutecolor.G);
  EEPROM.write(105, minutecolor.B);

  //write the blend point
  EEPROM.write(106, blendpoint);

  //write the brightness
  EEPROM.write(107, brightness);

  // Beta-Channel
  if (server.hasArg("betahidden")) {
    betaChannel = server.hasArg("betachannel");
    EEPROM.write(367, betaChannel ? 1 : 0);
  }


  // Heap freigeben vor dem Aufbau der Root-Seite
  yield();

  // index.html direkt aus LittleFS streamen - kein RAM fuer toSend noetig
  if (LittleFS.exists("/index.html")) {
    logTS(); dualOut.println("Sending handleRoot (LittleFS)");
    File f = LittleFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close();
    EEPROM.commit();
    return;
  }

  String toSend = FPSTR(root_html);
  toSend.replace("$menu", FPSTR(menu_html));
  String tempgradient = "";
  String csswgradient = "";
  const String scheme = "scheme";
  for (int i = 1; i < 5; i++) {
    //loop makes each of the save/load buttons coloured based on the scheme
    tempgradient = FPSTR(buttongradient_css);
    //load hour color
    tempcolor.R = EEPROM.read(100 + i * 15);
    tempcolor.G = EEPROM.read(101 + i * 15);
    tempcolor.B = EEPROM.read(102 + i * 15);
    //fix darkened colour schemes by manually lightening them.

    tempgradient.replace("$hourcolor", rgbToText(tempcolor));
    //load minute color
    tempcolor.R = EEPROM.read(103 + i * 15);
    tempcolor.G = EEPROM.read(104 + i * 15);
    tempcolor.B = EEPROM.read(105 + i * 15);

    tempgradient.replace("$minutecolor", rgbToText(tempcolor));

    tempgradient.replace("$scheme", scheme + i);

    csswgradient += tempgradient;
  }
  toSend.replace("$csswgradient", csswgradient);
  if (webMode != 2) {
    // don't send external links if we're local only
    toSend.replace("$externallinks", FPSTR(externallinks));

  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");

  }

  if (clockmode == alarm) {
    toSend.replace("$alarm", "<a class='btn btn-default' href=/?alarmoff=1>Cancel Alarm</a>");
  }
  else {
    toSend.replace("$alarm", "<a class='btn btn-default' href=/alarm>Alarm Einstellungen</a>");
  }
  toSend.replace("$minutecolor", rgbToText(minutecolor));
  toSend.replace("$hourcolor", rgbToText(hourcolor));
  toSend.replace("$blendpoint", String(int(blendpoint)));
  toSend.replace("$nightbrightness", String(int(nightBrightness)));
  toSend.replace("$maxBrightness", String(int(maxBrightness)));
  toSend.replace("$brightness", String(int(brightness)));
  toSend.replace("$firmware_version", firmware_version);
  
  // Prüfe ob die Seite korrekt aufgebaut wurde
  if (toSend.indexOf("$externallinks") >= 0 || toSend.indexOf("$csswgradient") >= 0) {
    logTS(); dualOut.print("[WARN] Root-Seite unvollstaendig! Heap: ");
    dualOut.println(ESP.getFreeHeap());
    // Nochmal versuchen mit freigegebenem Heap
    if (toSend.indexOf("$externallinks") >= 0) {
      if (webMode != 2) {
        toSend.replace("$externallinks", FPSTR(externallinks));
      } else {
        toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");
      }
    }
  }
  
  // index.html aus LittleFS liefern falls vorhanden (spart RAM)
  if (LittleFS.exists("/index.html")) {
    // Erst toSend leeren um RAM freizugeben
    toSend = "";
    File f = LittleFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close();
  } else {
    server.setContentLength(toSend.length());
    server.send(200, "text/html", "");
    server.sendContent(toSend);
    toSend = "";
  }
  logTS(); dualOut.println("Sending handleRoot");
  EEPROM.commit();
  delay(300);
}
