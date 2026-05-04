// m07_wifi.ino

void initWiFi() {
  dualOut.println();
  dualOut.println();
  logTS(); dualOut.println("Startup");
  esid.trim();
  if (webMode == 2) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid);
    //    WiFi.begin((char*) ssid.c_str()); // not sure if need but works
    //dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    //dnsServer.start(DNS_PORT, "*", apIP);
    logTS(); dualOut.println("USP Server started");
    logTS(); dualOut.print("Access point started with name ");
    dualOut.println(ssid);
    //server.on("/generate_204", handleRoot);  //Android captive
    server.onNotFound(handleRoot);
    launchWeb(2);
    return;

  }
  if (webMode == 1) {
    // test esid
    WiFi.persistent(false);
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    logTS(); dualOut.print("Connecting to WiFi ");
    dualOut.println(esid);
    WiFi.begin(esid.c_str(), epass.c_str());
    if ( testWifi() == 20 ) {
      launchWeb(1);
      return;
    }
  }
  logo();

  setupAP();
}

int testWifi(void) {
  int c = 0;
  logTS(); dualOut.println("Waiting for Wifi to connect");
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return (20);
    }
    delay(500);
    logTS(); dualOut.print(".");
    c++;
  }
  logTS(); dualOut.println("Connect timed out, opening AP");
  return (10);
}

void setupAP(void) {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  logTS(); dualOut.println("scan done");

  if (n == 0) {
    logTS(); dualOut.println("no networks found");
    st = "<label><input type='radio' name='ssid' value='No networks found' onClick='regularssid()'>No networks found</input></label><br>";
  } else {
    dualOut.print(n);
    logTS(); dualOut.println("Networks found");
    st = "";
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      dualOut.print(i + 1);
      logTS(); dualOut.print(": ");
      dualOut.print(WiFi.SSID(i));
      logTS(); dualOut.print(" (");
      dualOut.print(WiFi.RSSI(i));
      logTS(); dualOut.print(")");
      dualOut.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " (OPEN)" : "*");

      // Print to web SSID and RSSI for each network found
      st += "<label><input type='radio' name='ssid' value='";
      st += WiFi.SSID(i);
      st += "' onClick='regularssid()'>";
      st += i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " (OPEN)" : "*";
      st += "</input></label><br>";
      delay(10);
    }
    //st += "</ul>";
  }
  logTS(); dualOut.println("");
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid);
  //WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  logTS(); dualOut.println("USP Server started");
  logTS(); dualOut.print("Access point started with name ");
  dualOut.println(ssid);
  //WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  logTS(); dualOut.print("Access point started with name ");
  dualOut.println(ssid);
  launchWeb(0);
}

//------------------------------------------------------Web handle sections-------------------------------------------------------------------
void launchWeb(int webtype) {
  webMode = webtype;
  int clockname_len = clockname.length() + 1;
  char clocknamechar[clockname_len];
  logTS(); dualOut.println("");
  logTS(); dualOut.println("WiFi connected");
  switch (webtype) {
    case 0:
      //set up wifi network to connect to since we are in setup mode.
      webMode == 0;
      dualOut.println(WiFi.softAPIP());
      server.on("/", webHandleConfig);
      server.on("/a", webHandleConfigSave);
      server.on("/timezonesetup", webHandleTimeZoneSetup);
      server.on("/passwordinput", webHandlePassword);
      server.on("/clockmenustyle.css", handleCSS);
      server.on("/switchwebmode", webHandleSwitchWebMode);
      server.on("/generate_204", webHandleConfig);  //Android captive
      server.onNotFound(webHandleConfig);

      break;

    case 1:
      //setup DNS since we are a client in WiFi net

      // Expliziten DNS-Server setzen falls DHCP keinen liefert
      if (WiFi.dnsIP().toString() == "0.0.0.0") {
        WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8,8,8,8));
        logTS(); dualOut.println("DNS: Fallback auf 8.8.8.8");
      } else {
        logTS(); dualOut.print("DNS: ");
        dualOut.println(WiFi.dnsIP().toString());
      }

      clockname.toCharArray(clocknamechar, clockname_len);
      if (!mdns.begin(clocknamechar)) {
        logTS(); dualOut.println("MDNS Fehler - wird beim Neustart aktiv.");
        // kein Endlosloop bei MDNS-Fehler
      } else {
        logTS(); dualOut.println("mDNS responder started");
      }

      dualOut.printf("Starting SSDP...\n");
      SSDP.setSchemaURL("description.xml");
      SSDP.setHTTPPort(80);
      SSDP.setName("MikoTec LED Uhr");
      SSDP.setSerialNumber("4");
      SSDP.setURL("index");
      SSDP.setModelName("MikoTec LED Uhr v1");
      SSDP.setModelNumber("4");
      SSDP.setModelURL("http://www.mikotec-led-uhr.de");
      SSDP.setManufacturer("MikoTec");
      SSDP.setManufacturerURL("http://www.mikotec-led-uhr.de");
      SSDP.begin();

      dualOut.println(WiFi.localIP());
      setUpServerHandle();

      break;

    case 2:
      //direct control over clock through it's own wifi network
      setUpServerHandle();

      break;

  }
  if (webtype == 0) {


  } else {

  }

  //server.onNotFound(webHandleRoot);

  // LittleFS initialisieren
  if (LittleFS.begin()) {
    logTS(); dualOut.println("[LittleFS] Dateisystem bereit");
    // LittleFS-Version ausgeben
    if (LittleFS.exists("/fs_version.txt")) {
      File fv = LittleFS.open("/fs_version.txt", "r");
      String fsVer = fv.readString();
      fv.close();
      logTS(); dualOut.println("[LittleFS] FS-Version: " + fsVer);
    }
    server.serveStatic("/style.css",   LittleFS, "/style.css",   "max-age=3600");
    server.serveStatic("/menu.css",    LittleFS, "/menu.css",    "max-age=3600");
    server.serveStatic("/clock.js",    LittleFS, "/clock.js",    "max-age=3600");
    server.serveStatic("/menu.js",     LittleFS, "/menu.js",     "max-age=3600");
    server.serveStatic("/settings.js", LittleFS, "/settings.js", "max-age=3600");
    server.serveStatic("/support.js",  LittleFS, "/support.js",  "max-age=3600");
    server.serveStatic("/timezone.js", LittleFS, "/timezone.js", "max-age=3600");
    server.serveStatic("/colour.js",   LittleFS, "/colour.js",   "max-age=3600");
    // Log-Datei: alte log.txt -> log_prev.txt, neue log.txt oeffnen
    if (LittleFS.exists("/log.txt")) {
      if (LittleFS.exists("/log_prev.txt")) LittleFS.remove("/log_prev.txt");
      LittleFS.rename("/log.txt", "/log_prev.txt");
    }
    logFile = LittleFS.open("/log.txt", "w");
    if (logFile) {
      logFSready = true;
      // Bisherigen RAM-Buffer in Datei schreiben
      String buf = getLogContent();
      logFile.print(buf);
      logFile.flush();
      logTS(); dualOut.println("[LittleFS] Log-Datei geoeffnet: /log.txt");
    } else {
      logTS(); dualOut.println("[LittleFS] WARN: Log-Datei konnte nicht geoeffnet werden");
    }
    server.serveStatic("/log.txt",      LittleFS, "/log.txt");
    server.serveStatic("/log_prev.txt", LittleFS, "/log_prev.txt");
  } else {
    logTS(); dualOut.println("[LittleFS] FEHLER: Dateisystem nicht gefunden!");
  }

  server.begin();
  logTS(); dualOut.println("Web server started");
  //Store global to use in loop()
}
