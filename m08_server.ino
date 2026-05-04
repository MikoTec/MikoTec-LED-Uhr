// m08_server.ino

void setUpServerHandle() {
  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.on("/description.xml", ssdpResponder);
  server.on("/cleareeprom", webHandleClearRom);
  server.on("/cleareepromsure", webHandleClearRomSure);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/settings", HTTP_POST, handleSettings);
  server.on("/timezone", handleTimezone);
  server.on("/clockmenustyle.css", handleCSS);
  server.on("/spectrum.css", handlespectrumCSS);
  server.on("/spectrum.js", handlespectrumjs);
  server.on("/Colour.js", handlecolourjs);
  server.on("/clock.js", handleclockjs);
  server.on("/switchwebmode", webHandleSwitchWebMode);
  server.on("/nightmodedemo", webHandleNightModeDemo);
  server.on("/timeset", webHandleTimeSet);
  server.on("/gettime", handleGetTime);
  server.on("/getstate", handleGetState);
  server.on("/getsettings", handleGetSettings);
  server.on("/alarm", webHandleAlarm);
  server.on("/reflection", webHandleReflection);
  server.on("/dawn", webHandleDawn);
  server.on("/moon", webHandleMoon);
  server.on("/brighttest", brighttest);
  server.on("/lightup", lightup);
  server.on("/game", webHandleGame);
  server.on("/hilfe", handleHilfe);
  server.on("/support", handleSupport);
  server.on("/getlog", handleGetLog);
  server.on("/getsysinfo", handleGetSysInfo);
  server.on("/reboot", handleReboot);
  server.on("/speed",speedup);
  server.on("/getmqtt", handleGetMqtt);
  server.on("/setmqtt", HTTP_POST, handleSetMqtt);

  // Update-Log Dateien zum Download anbieten
  server.on("/last_fw_update.txt", [](){
    if (LittleFS.exists("/last_fw_update.txt")) {
      File f = LittleFS.open("/last_fw_update.txt", "r");
      server.streamFile(f, "text/plain");
      f.close();
    } else {
      server.send(404, "text/plain", "Keine Firmware-Update-Logdatei vorhanden");
    }
  });
  server.on("/last_fs_update.txt", [](){
    if (LittleFS.exists("/last_fs_update.txt")) {
      File f = LittleFS.open("/last_fs_update.txt", "r");
      server.streamFile(f, "text/plain");
      f.close();
    } else {
      server.send(404, "text/plain", "Keine Dateisystem-Update-Logdatei vorhanden");
    }
  });

  // Deutsche Update-Seite (überschreibt den Standard-Handler von httpUpdater)
  server.on("/update", HTTP_GET, [](){
    String upd = "<!DOCTYPE html><html lang=\'de\'><head>"
      "<meta charset=\'utf-8\'>"
      "<meta name=\'viewport\' content=\'width=device-width,initial-scale=1\'/>"
      "<title>Firmware Update</title>"
      "<link rel=stylesheet href=\'clockmenustyle.css\'>"
      "</head><body class=\'settings-page\'>";
    upd += FPSTR(menu_html);
    upd += "<div id=\'rcorners2\' style=\'max-width:420px;margin:30px auto;text-align:center;\'>"
      "<label class=\'section-head\'>Firmware Update</label>"
      "<p class=\'version\' style=\'margin:10px 0;color:#888;font-size:0.9em;\'>Installierte Version: " + String(firmware_version) + "</p>"
      "<form method=\'POST\' enctype=\'multipart/form-data\' style=\'margin-top:20px;\'>"
      "<ul class=\'form-verticle\'>"
      "<li><label>Firmware-Datei auswaehlen (.bin)</label>"
      "<div class=\'form-field\'><input type=\'file\' accept=\'.bin,.bin.gz\' name=\'firmware\'></div></li>"
      "</ul>"
      "<input class=\'btn btn-green\' type=\'submit\' value=\'Firmware aktualisieren\'>"
      "</form>"
      "</div>"
      "</body></html>";
    server.send(200, "text/html", upd);
  });

  // httpUpdater registriert nur noch den POST-Handler
  httpUpdater.setup(&server);

  // Callback: Log sichern bevor Browser-FW-Update startet
  Update.onStart([]() {
    logTS(); dualOut.println("[UPDATE] Browser-Update startet, sichere Log...");
    if (logFSready && logFile) {
      logFile.flush();
      logFile.close();
      logFSready = false;
    }
    // Log als last_fw_update.txt speichern (LittleFS ueberlebt FW-Update)
    String fullLog = getLogContent();
    if (LittleFS.exists("/last_fw_update.txt")) LittleFS.remove("/last_fw_update.txt");
    File fwLog = LittleFS.open("/last_fw_update.txt", "w");
    if (fwLog) {
      fwLog.print(fullLog);
      fwLog.close();
    }
    // Log per MQTT publishen
    if (mqttEnabled && mqttClient.connected()) {
      String base = mqttBaseTopic() + "/log_backup";
      int chunks = (fullLog.length() + 1023) / 1024;
      for (int i = 0; i < chunks && i < 10; i++) {
        String chunk = fullLog.substring(i * 1024, _min((int)fullLog.length(), (i + 1) * 1024));
        mqttClient.publish((base + "/" + String(i)).c_str(), chunk.c_str(), true);
        mqttClient.loop();
        delay(50);
      }
    }
  });

  // LittleFS Dateisystem Update per Browser
  server.on("/update_fs", HTTP_GET, [](){
    String p = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'/>"
      "<title>Dateisystem Update</title>"
      "<link rel=stylesheet href='/style.css'>"
      "<link rel=stylesheet href='/menu.css'>"
      "</head><body class='settings-page'>"
      "<div id='menu-placeholder'></div>"
      "<div id='rcorners2' style='max-width:480px;margin:30px auto;text-align:center;'>"
      "<label class='section-head'>Dateisystem Update</label>"
      "<p style='margin:10px 0;color:#888;font-size:0.9em;'>Firmware: " + String(firmware_version) + "</p>"
      "<p style='font-size:13px;color:#555;margin-bottom:16px;'>Laedt HTML, CSS und JS Dateien ins Flash-Dateisystem.<br>Danach startet die Uhr neu.</p>"
      "<form method='POST' enctype='multipart/form-data'>"
      "<ul class='form-verticle'>"
      "<li><label>LittleFS-Image auswaehlen (.bin)</label>"
      "<div class='form-field'><input type='file' accept='.bin' name='filesystem'></div></li>"
      "</ul>"
      "<input class='btn btn-default' type='submit' value='Dateisystem aktualisieren'>"
      "</form></div>"
      "<script src='/menu.js'></script>"
      "</body></html>";
    server.send(200, "text/html", p);
  });

  static bool fsUploadError = false;
  static uint32_t fsWriteAddr = 0;
  server.on("/update_fs", HTTP_POST,
    [](){
      server.sendHeader("Connection", "close");
      if (fsUploadError) {
        server.send(500, "text/plain", "Fehler beim Dateisystem-Update!");
      } else {
        // Log-Inhalt vor Neustart sichern
        static String fsOtaLogSnapshot;
        fsOtaLogSnapshot = getLogContent();
        fsOtaLogSnapshot += "\n[FS-OTA] Upload abgeschlossen - Log gesichert vor Neustart\n";
        // Log per MQTT sichern
        if (mqttEnabled && mqttClient.connected()) {
          String base = mqttBaseTopic() + "/log_backup";
          int chunks = (fsOtaLogSnapshot.length() + 1023) / 1024;
          for (int i = 0; i < chunks && i < 10; i++) {
            String chunk = fsOtaLogSnapshot.substring(i * 1024, _min((int)fsOtaLogSnapshot.length(), (i + 1) * 1024));
            mqttClient.publish((base + "/" + String(i)).c_str(), chunk.c_str(), true);
            mqttClient.loop();
            delay(50);
          }
        }
        // last_fs_update.txt ins neue FS schreiben (FS wurde gerade geflasht)
        if (LittleFS.begin()) {
          File fsLog = LittleFS.open("/last_fs_update.txt", "w");
          if (fsLog) {
            fsLog.print(fsOtaLogSnapshot);
            fsLog.close();
          }
        }
        // Handler fuer Log-Download registrieren (kein LittleFS noetig)
        server.on("/fs_ota_log", [](){
          server.sendHeader("Content-Disposition", "attachment; filename=fs_ota_log.txt");
          server.send(200, "text/plain", fsOtaLogSnapshot);
        });
        // Handler fuer Neustart registrieren
        server.on("/fs_ota_restart", [](){
          server.send(200, "text/html",
            "<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta http-equiv='refresh' content='10;url=/'></head>"
            "<body style='font-family:sans-serif;text-align:center;margin-top:60px;'>"
            "<h2>Uhr startet neu...</h2>"
            "<p>Weiterleitung in 10 Sekunden</p>"
            "</body></html>");
          delay(500);
          ESP.restart();
        });
        // Erfolgsseite komplett inline - kein CSS/JS aus LittleFS (ist nicht mehr gemountet)
        String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
          "<meta name='viewport' content='width=device-width,initial-scale=1'>"
          "<style>"
          "body{font-family:Abel,sans-serif;background:#f5f5f5;margin:0;padding:20px;}"
          "h2{color:#4CAF50;letter-spacing:2px;text-transform:uppercase;}"
          ".box{background:#fff;max-width:480px;margin:40px auto;padding:30px;border-radius:8px;text-align:center;box-shadow:0 2px 8px rgba(0,0,0,0.1);}"
          ".btn{display:inline-block;margin:8px;padding:10px 20px;border-radius:4px;text-decoration:none;font-family:Abel,sans-serif;letter-spacing:1px;text-transform:uppercase;font-size:14px;cursor:pointer;}"
          ".btn-green{background:#4CAF50;color:#fff;}"
          ".btn-red{background:#e53935;color:#fff;}"
          ".btn-default{background:#333;color:#fff;}"
          "</style></head>"
          "<body><div class='box'>"
          "<h2>Dateisystem aktualisiert!</h2>"
          "<p>Log vor dem Neustart sichern, dann Uhr neu starten.</p>"
          "<a class='btn btn-default' href='/fs_ota_log'>Log herunterladen</a><br>"
          "<a class='btn btn-red' href='/fs_ota_restart'>Uhr neu starten</a>"
          "</div></body></html>";
        server.send(200, "text/html", html);
      }
    },
    [](){
      extern uint32_t _FS_start;
      extern uint32_t _FS_end;
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        logTS(); dualOut.println("[FS-OTA] Start");
        // Log-Datei flushen und schliessen bevor LittleFS beendet wird
        if (logFSready && logFile) {
          logFile.flush();
          logFile.close();
          logFSready = false;
        }
        LittleFS.end();
        fsUploadError = false;
        // Physikalische Flash-Adresse aus Linker-Symbol (mapped Adresse - 0x40200000)
        fsWriteAddr = ((uint32_t)&_FS_start) - 0x40200000;
        uint32_t fsSize = ((uint32_t)&_FS_end) - ((uint32_t)&_FS_start);
        logTS(); dualOut.print("[FS-OTA] Partition: 0x");
        dualOut.print(fsWriteAddr, HEX);
        dualOut.print(" Groesse: ");
        dualOut.println(fsSize);
        // Kein Vorab-Loeschen mehr - Sektoren werden einzeln vor dem Schreiben geloescht
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!fsUploadError) {
          uint32_t len = upload.currentSize;
          uint8_t* buf = upload.buf;
          // Puffer auf 4 Bytes auffullen falls noetig
          uint8_t aligned[HTTP_UPLOAD_BUFLEN + 4];
          memcpy(aligned, buf, len);
          uint32_t alignedLen = len;
          while (alignedLen % 4 != 0) aligned[alignedLen++] = 0xFF;
          // Sektor loeschen wenn wir an dessen Anfang sind
          if (fsWriteAddr % 4096 == 0) {
            ESP.flashEraseSector(fsWriteAddr / 4096);
          }
          if (!ESP.flashWrite(fsWriteAddr, (uint32_t*)aligned, alignedLen)) {
            fsUploadError = true;
            logTS(); dualOut.println("[FS-OTA] flashWrite Fehler!");
          }
          fsWriteAddr += alignedLen;
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (!fsUploadError) {
          logTS(); dualOut.print("[FS-OTA] OK: ");
          dualOut.print(upload.totalSize); dualOut.println(" Bytes");
        }
      }
    }
  );

  server.begin();

}


void speedup() {
  speed++;
  speed = speed%3;
  server.send(200, "text/html", "Speed Up: " + speed);

}


void webHandleSwitchWebMode() {
  logTS(); dualOut.println("Sending webHandleSwitchWebMode");
  if ((webMode == 0) || (webMode == 1)) {
    webMode = 2;
    server.send(200, "text/html", "webMode set to 2");
  } else {
    webMode = 1;
    server.send(200, "text/html", "webMode set to 1");
  }
  EEPROM.begin(512);
  delay(10);
  EEPROM.write(186, webMode);
  dualOut.println(webMode);
  EEPROM.commit();
  delay(1000);
  EEPROM.end();

  ESP.reset();


}

void webHandleConfig() {
  lastInteraction = millis();
  logTS(); dualOut.println("Sending webHandleConfig");

  // Chunked streaming statt großem String — verhindert OOM bei vielen WLANs
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  // HTML-Kopf aus PROGMEM
  String head = F("<!DOCTYPE HTML><html><head>"
    "<link rel=stylesheet href='clockmenustyle.css'>"
    "<meta http-equiv=Content-Type content=\"text/html; charset=utf-8\" />"
    "<meta name=viewport content=\"width=device-width, initial-scale=1.0\">"
    "</head><body class=settings-page>"
    "<strong>Netzwerk </strong>-> Passwort -> Zeitzone<BR>"
    "<h1>Netzwerk waehlen</h1>"
    "<form class=form-verticle action=/passwordinput method=GET><ul>");
  server.sendContent(head);

  // SSID-Liste direkt streamen ohne globalen st-String
  int n = WiFi.scanComplete();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      String entry = "<label><input type='radio' name='ssid' value='";
      entry += WiFi.SSID(i);
      entry += "' onClick='regularssid()'>";
      entry += String(i + 1) + ": " + WiFi.SSID(i);
      entry += " (" + String(WiFi.RSSI(i)) + ")";
      entry += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " (OPEN)" : "*";
      entry += "</input></label><br>";
      server.sendContent(entry);
    }
  } else {
    server.sendContent(st); // Fallback auf alten st-String
  }

  // HTML-Fuß
  String foot = F("<label onClick=otherssid()>"
    "<input type=radio name=ssid id=other_ssid value=other>andere SSID:</input>"
    "<input type=text name=other id=\"other_text\"/></label><br>"
    "<input type=submit value=Weiter>"
    "</form></body>"
    "<script>function otherssid(){a=document.getElementById(\"other_ssid\");a.checked=true}"
    "function regularssid(){a=document.getElementById(\"other_text\");a.value=\"\"};</script>"
    "</html>");
  server.sendContent(foot);
  server.sendContent("");
}

void webHandlePassword() {
  logTS(); dualOut.println("Sending webHandlePassword");


  String toSend = FPSTR(password_html);
  //toSend.replace("$css", css_file);

  server.send(200, "text/html", toSend);

  String qsid;
  if (server.arg("ssid") == "other") {
    qsid = server.arg("other");
  } else {
    qsid = server.arg("ssid");
  }
  cleanASCII(qsid);

  dualOut.println(qsid);
  logTS(); dualOut.println("");
  logTS(); dualOut.println("clearing old ssid.");
  clearssid();
  EEPROM.begin(512);
  delay(10);
  logTS(); dualOut.println("writing eeprom ssid.");
  //addr += EEPROM.put(addr, qsid);
  for (int i = 0; i < qsid.length(); ++i)
  {
    EEPROM.write(i, qsid[i]);
    dualOut.print(qsid[i]);
  }
  logTS(); dualOut.println("");
  EEPROM.commit();
  delay(1000);
  EEPROM.end();

}

void cleanASCII(String &input) {
  input.replace("%21", "!");
  input.replace("%22", "\"");
  input.replace("%23", "#");
  input.replace("%24", "$");
  input.replace("%25", "%");
  input.replace("%26", "&");
  input.replace("%27", "'");
  input.replace("%28", "(");
  input.replace("%29", ")");
  input.replace("%2A", "*");
  input.replace("%2B", "+");
  input.replace("%2C", ",");
  input.replace("%2D", "-");
  input.replace("%2E", ".");
  input.replace("%2F", "/");
  input.replace("%3A", ":");
  input.replace("%3B", ";");
  input.replace("%3C", "<");
  input.replace("%3D", "=");
  input.replace("%3E", ">");
  input.replace("%3F", "?");
  input.replace("%40", "@");
  input.replace("%5B", "[");
  input.replace("%5D", "]");
  input.replace("%5E", "^");
  input.replace("%5F", "_");
  input.replace("%60", "`");
  input.replace("%7B", "{");
  input.replace("%7C", "|");
  input.replace("%7D", "}");
  input.replace("%7E", "~");
  input.replace("%7F", "");
  input.replace("+", " ");

}

void webHandleTimeZoneSetup() {
  logTS(); dualOut.println("Sending webHandleTimeZoneSetup");

  // Passwort aus Request verarbeiten (muss vor send() passieren)
  logTS(); dualOut.println("clearing old pass.");
  clearpass();
  String qpass = server.arg("pass");
  cleanASCII(qpass);
  dualOut.println(qpass);
  EEPROM.begin(512);
  delay(10);
  logTS(); dualOut.println("writing eeprom pass.");
  for (int i = 0; i < qpass.length(); ++i) {
    EEPROM.write(32 + i, qpass[i]);
    dualOut.print(qpass[i]);
  }
  dualOut.println("");
  EEPROM.write(186, 1);
  EEPROM.commit();
  delay(100);
  EEPROM.end();

  // Chunked streaming statt FPSTR-String mit 82 Ersetzungen — verhindert OOM
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(F("<!DOCTYPE HTML><html><head><title>Zeitzone</title>"
    "<link rel=stylesheet href=\"clockmenustyle.css\">"
    "<meta http-equiv=Content-Type content=\"text/html; charset=utf-8\" />"
    "<meta name=viewport content=\"width=device-width, initial-scale=1.0\">"
    "</head><body class=settings-page>"
    "<strong>Netzwerk -> Passwort -> </strong>Zeitzone<BR>"
    "<h1>Zeitzone waehlen</h1>"
    "<form class=form-verticle action=/a method=GET>"
    "<label>Zeitzone</label>"
    "<select name=timezone>"));

  // Zeitzonen-Optionen direkt streamen
  const char* tzNames[] = {
    "(GMT-12:00) International Date Line West","(GMT-11:00) Midway Island, Samoa",
    "(GMT-10:00) Hawaii","(GMT-09:00) Alaska","(GMT-08:00) Pacific Time (US & Canada)",
    "(GMT-08:00) Tijuana, Baja California","(GMT-07:00) Arizona",
    "(GMT-07:00) Chihuahua, La Paz, Mazatlan","(GMT-07:00) Mountain Time (US & Canada)",
    "(GMT-06:00) Central America","(GMT-06:00) Central Time (US & Canada)",
    "(GMT-06:00) Guadalajara, Mexico City, Monterrey","(GMT-06:00) Saskatchewan",
    "(GMT-05:00) Bogota, Lima, Quito, Rio Branco","(GMT-05:00) Eastern Time (US & Canada)",
    "(GMT-05:00) Indiana (East)","(GMT-04:00) Atlantic Time (Canada)",
    "(GMT-04:00) Caracas, La Paz","(GMT-04:00) Manaus","(GMT-04:00) Santiago",
    "(GMT-03:30) Newfoundland","(GMT-03:00) Brasilia",
    "(GMT-03:00) Buenos Aires, Georgetown","(GMT-03:00) Greenland",
    "(GMT-03:00) Montevideo","(GMT-02:00) Mid-Atlantic","(GMT-01:00) Cape Verde Is.",
    "(GMT-01:00) Azores","(GMT+00:00) Casablanca, Monrovia, Reykjavik",
    "(GMT+00:00) Greenwich Mean Time","(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna",
    "(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague",
    "(GMT+01:00) Brussels, Copenhagen, Madrid, Paris",
    "(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb","(GMT+01:00) West Central Africa",
    "(GMT+02:00) Amman","(GMT+02:00) Athens, Bucharest, Istanbul","(GMT+02:00) Beirut",
    "(GMT+02:00) Cairo","(GMT+02:00) Harare, Pretoria",
    "(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius",
    "(GMT+02:00) Jerusalem","(GMT+02:00) Minsk","(GMT+02:00) Windhoek",
    "(GMT+03:00) Kuwait, Riyadh, Baghdad","(GMT+03:00) Moscow, St. Petersburg, Volgograd",
    "(GMT+03:00) Nairobi","(GMT+03:00) Tbilisi","(GMT+03:30) Tehran",
    "(GMT+04:00) Abu Dhabi, Muscat","(GMT+04:00) Baku","(GMT+04:00) Yerevan",
    "(GMT+04:30) Kabul","(GMT+05:00) Yekaterinburg",
    "(GMT+05:00) Islamabad, Karachi, Tashkent","(GMT+05:30) Sri Jayawardenapura",
    "(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi","(GMT+05:45) Kathmandu",
    "(GMT+06:00) Almaty, Novosibirsk","(GMT+06:00) Astana, Dhaka",
    "(GMT+06:30) Yangon (Rangoon)","(GMT+07:00) Bangkok, Hanoi, Jakarta",
    "(GMT+07:00) Krasnoyarsk","(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi",
    "(GMT+08:00) Kuala Lumpur, Singapore","(GMT+08:00) Irkutsk, Ulaan Bataar",
    "(GMT+08:00) Perth","(GMT+08:00) Taipei","(GMT+09:00) Osaka, Sapporo, Tokyo",
    "(GMT+09:00) Seoul","(GMT+09:00) Yakutsk","(GMT+09:30) Adelaide",
    "(GMT+09:30) Darwin","(GMT+10:00) Brisbane","(GMT+10:00) Canberra, Melbourne, Sydney",
    "(GMT+10:00) Hobart","(GMT+10:00) Guam, Port Moresby","(GMT+10:00) Vladivostok",
    "(GMT+11:00) Magadan, Solomon Is., New Caledonia","(GMT+12:00) Auckland, Wellington",
    "(GMT+12:00) Fiji, Kamchatka, Marshall Is.","(GMT+13:00) Nuku'alofa"
  };

  for (int i = 0; i < 82; i++) {
    String opt = "<option value=\"" + String(i+1) + "\"";
    if (timezonevalue == i+1) opt += " selected";
    opt += ">" + String(tzNames[i]) + "</option>";
    server.sendContent(opt);
  }

  server.sendContent(F("</select>"
    "<br><label>Breitengrad</label>"
    "<input type=text name=latitude value=\""));
  server.sendContent(String(latitude));
  server.sendContent(F("\"><br><label>Laengengrad</label>"
    "<input type=text name=longitude value=\""));
  server.sendContent(String(longitude));
  server.sendContent(F("\"><br>"
    "<input type=submit value=Weiter>"
    "</form></body></html>"));
  server.sendContent("");
}

void webHandleConfigSave() {
  lastInteraction = millis();
  logTS(); dualOut.println("Sending webHandleConfigSave");
  // /a?ssid=blahhhh&pass=poooo
  String s;
  s = "<p>Settings saved to memory. Clock will now restart and you can find it on your local WiFi network. <p>Please reconnect your phone to your WiFi network first</p>\r\n\r\n";
  server.send(200, "text/html", s);
  EEPROM.begin(512);
  if (server.hasArg("timezone")) {
    String timezonestring = server.arg("timezone");
    timezonevalue = timezonestring.toInt();//atoi(c);
    interpretTimeZone(timezonevalue);
    EEPROM.write(179, timezonevalue);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }

  if (server.hasArg("DST")) {
    DSTtime = 1;
    EEPROM.write(192, 1);
  }
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

    }
    brightness = maxBrightness;
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
  }

  if (server.hasArg("latitude")) {
    String latitudestring = server.arg("latitude");  //get value from blend slider
    latitude = latitudestring.toFloat();//atoi(c);  //get value from html5 color element
    writeLatLong(175, latitude);
  }
  if (server.hasArg("longitude")) {
    String longitudestring = server.arg("longitude");  //get value from blend slider
    longitude = longitudestring.toFloat();//atoi(c);  //get value from html5 color element
    writeLatLong(177, longitude);
    DSTauto = 1;
    EEPROM.write(185, 1);
    EEPROM.write(179, timezonevalue);  // Fix: timezonevalue (int) statt timezone (float)
  }
  EEPROM.commit();
  delay(1000);
  EEPROM.end();
  logTS(); dualOut.println("Settings written, restarting!");
  ESP.reset();
}
