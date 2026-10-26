// m04_ota.ino

// Vergleicht zwei 4-stellige Versionsstrings (a.b.c.d).
// Liefert true, wenn remoteVersion ECHT neuer ist als localVersion.
bool isVersionNewer(const String& remoteVersion, const String& localVersion) {
  int lv[4] = {0, 0, 0, 0};
  int rv[4] = {0, 0, 0, 0};
  
  sscanf(localVersion.c_str(), "%d.%d.%d.%d", &lv[0], &lv[1], &lv[2], &lv[3]);
  sscanf(remoteVersion.c_str(), "%d.%d.%d.%d", &rv[0], &rv[1], &rv[2], &rv[3]);
  
  for (int i = 0; i < 4; i++) {
    if (rv[i] > lv[i]) return true;
    if (rv[i] < lv[i]) return false;
  }
  return false;
}

bool isNewerVersion(const String& remoteVersion) {
  return isVersionNewer(remoteVersion, String(firmware_version));
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    logTS(); dualOut.println("[OTA] Kein WLAN verbunden, Update-Check uebersprungen.");
    return;
  }
  
  dualOut.println("==============================");
  logTS(); dualOut.println("[OTA] Pruefe auf Firmware-Update...");
  logTS(); dualOut.print("[OTA] Freier Heap: ");
  dualOut.print(ESP.getFreeHeap());
  dualOut.println(" Bytes");
  logTS(); dualOut.print("[OTA] Abfrage URL: ");
  dualOut.println(update_version_url);
  
  // MQTT vor dem TLS-Handshake trennen, um Heap-Peak zu entlasten
  // (loop() reconnectet automatisch; mqttDiscoverySent bleibt true -> keine erneute Discovery)
  if (mqttEnabled && mqttClient.connected()) {
    mqttClient.disconnect();
    logTS(); dualOut.println("[OTA] MQTT fuer TLS-Handshake getrennt");
  }
  delay(50);

  // --- version.json-Check in eigenem Scope ---
  // Reiner HTTP-Client (Port 80, kein TLS): kein BearSSL-Handshake, daher kein
  // Heap-Engpass. Der Client wird am Blockende freigegeben.
  String payload;
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, update_version_url);
    http.setTimeout(15000);

    logTS(); dualOut.println("[OTA] Sende HTTP-Anfrage...");
    int httpCode = http.GET();

    logTS(); dualOut.print("[OTA] HTTP-Antwort: ");
    dualOut.println(httpCode);

    // Diagnose bei Verbindungsfehler: Heap ausgeben
    if (httpCode < 0) {
      logTS(); dualOut.print("[OTA] Verbindungsfehler, freier Heap: ");
      dualOut.println(ESP.getFreeHeap());
    }

    if (httpCode != 200) {
      logTS(); dualOut.print("[OTA] Update-Check fehlgeschlagen, HTTP-Code: ");
      dualOut.println(httpCode);
      http.end();
      dualOut.println("==============================");
      return;
    }

    logTS(); dualOut.println("[OTA] version.json erfolgreich geladen.");
    payload = http.getString();
    http.end();
  }  // Scope-Ende: Check-Client + TLS-Kontext hier freigegeben
  logTS(); dualOut.print("[OTA] Inhalt: ");
  dualOut.println(payload);
  
  // Prefix bestimmen: stable oder beta
  String prefix = betaChannel ? "beta" : "stable";
  logTS(); dualOut.println("[OTA] Update-Kanal: " + prefix);
  
  // Firmware-Version aus JSON parsen
  String vKey = "\"" + prefix + "_version\"";
  int vStart = payload.indexOf(vKey);
  if (vStart == -1) {
    logTS(); dualOut.println("[OTA] FEHLER: version.json ungueltig - kein " + prefix + "_version Feld gefunden.");
    dualOut.println("==============================");
    return;
  }
  vStart = payload.indexOf("\"", vStart + vKey.length()) + 1;
  int vEnd = payload.indexOf("\"", vStart);
  String remoteVersion = payload.substring(vStart, vEnd);
  
  // Firmware-Dateiname aus JSON parsen
  String fKey = "\"" + prefix + "_file\"";
  int fStart = payload.indexOf(fKey);
  if (fStart == -1) {
    logTS(); dualOut.println("[OTA] FEHLER: version.json ungueltig - kein " + prefix + "_file Feld gefunden.");
    dualOut.println("==============================");
    return;
  }
  fStart = payload.indexOf("\"", fStart + fKey.length()) + 1;
  int fEnd = payload.indexOf("\"", fStart);
  String remoteFile = payload.substring(fStart, fEnd);
  
  // LittleFS-Version aus JSON parsen
  String remoteFsVersion = "";
  String remoteFsFile = "";
  String fsKey = "\"" + prefix + "_fs_version\"";
  int fsStart = payload.indexOf(fsKey);
  if (fsStart != -1) {
    fsStart = payload.indexOf("\"", fsStart + fsKey.length()) + 1;
    int fsEnd = payload.indexOf("\"", fsStart);
    remoteFsVersion = payload.substring(fsStart, fsEnd);
  }
  String ffKey = "\"" + prefix + "_fs_file\"";
  int ffStart = payload.indexOf(ffKey);
  if (ffStart != -1) {
    ffStart = payload.indexOf("\"", ffStart + ffKey.length()) + 1;
    int ffEnd = payload.indexOf("\"", ffStart);
    remoteFsFile = payload.substring(ffStart, ffEnd);
  }
  
  logTS(); dualOut.print("[OTA] Installierte Version: ");
  dualOut.println(firmware_version);
  logTS(); dualOut.print("[OTA] Verfuegbare Version:  ");
  dualOut.println(remoteVersion);
  logTS(); dualOut.print("[OTA] Dateiname:            ");
  dualOut.println(remoteFile);
  
  // Lokale FS-Version lesen
  String localFsVersion = "";
  if (LittleFS.exists("/fs_version.txt")) {
    File fv = LittleFS.open("/fs_version.txt", "r");
    localFsVersion = fv.readString();
    localFsVersion.trim();
    fv.close();
  }
  logTS(); dualOut.print("[OTA] Installierte FS-Version: ");
  dualOut.println(localFsVersion.length() > 0 ? localFsVersion : "(unbekannt)");
  logTS(); dualOut.print("[OTA] Verfuegbare FS-Version:  ");
  dualOut.println(remoteFsVersion.length() > 0 ? remoteFsVersion : "(keine)");
  
  // MQTT Update-Status speichern und publishen
  mqttAvailableVersion = remoteVersion;
  mqttAvailableFile = remoteFile;
  mqttPublishUpdateState();
  
  // --- Firmware-Update ---
  if (isNewerVersion(remoteVersion)) {
    String binUrl = String(update_bin_base_url) + remoteFile;
    logTS(); dualOut.println("[OTA] *** Neue Firmware gefunden! ***");
    logTS(); dualOut.print("[OTA] Download URL: ");
    dualOut.println(binUrl);
    logTS(); dualOut.print("[OTA] Freier Heap vor Update: ");
    dualOut.print(ESP.getFreeHeap());
    dualOut.println(" Bytes");
    logTS(); dualOut.println("[OTA] Starte Firmware-Download und Flash...");
    
    // Log sichern bevor der ESP neu startet
    if (logFSready && logFile) {
      logFile.flush();
      logFile.close();
      logFSready = false;
      if (LittleFS.exists("/log_prev.txt")) LittleFS.remove("/log_prev.txt");
      LittleFS.rename("/log.txt", "/log_prev.txt");
    }
    
    // Hinweis: Log-Sicherung nach /last_fw_update.txt und MQTT-Log-Versand
    // wurden hier ENTFERNT - getLogContent() drueckte den Heap vor dem Download
    // auf ~5.7KB, wodurch der TLS-Handshake fuer den Download scheiterte (-1).
    // Das Log bleibt in /log_prev.txt erhalten (ueberlebt das Firmware-Update).
    
    // MQTT vor dem TLS-Handshake trennen (Heap-Peak entlasten)
    if (mqttEnabled && mqttClient.connected()) {
      mqttClient.disconnect();
      logTS(); dualOut.println("[OTA] MQTT fuer Firmware-Download getrennt");
    }
    delay(50);

    WiFiClient updateClient;
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    // Signaturpruefung NUR fuer diesen Auto-OTA-Download (MITM-Schutz ueber
    // HTTP). Objekte transient angelegt -> im Normalbetrieb kein Heap-Verbrauch.
    BearSSL::PublicKey* otaPubKey = new BearSSL::PublicKey(ota_pubkey);
    BearSSL::HashSHA256* otaHash = new BearSSL::HashSHA256();
    BearSSL::SigningVerifier* otaVerifier = new BearSSL::SigningVerifier(otaPubKey);
    Update.installSignature(otaHash, otaVerifier);
    logTS(); dualOut.print("[OTA] Signaturpruefung aktiv, freier Heap: ");
    dualOut.println(ESP.getFreeHeap());
    t_httpUpdate_return ret = ESPhttpUpdate.update(updateClient, binUrl);
    Update.installSignature(nullptr, nullptr);
    delete otaVerifier; delete otaHash; delete otaPubKey;
    
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        logTS(); dualOut.print("[OTA] FEHLER: Firmware-Update fehlgeschlagen (");
        dualOut.print(ESPhttpUpdate.getLastError());
        dualOut.print("): ");
        dualOut.println(ESPhttpUpdate.getLastErrorString());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        logTS(); dualOut.println("[OTA] Server meldet: Kein Update verfuegbar.");
        break;
      case HTTP_UPDATE_OK:
        logTS(); dualOut.println("[OTA] Firmware-Update erfolgreich! Neustart...");
        break;
    }
    // Nach erfolgreichem Firmware-Update startet der ESP neu,
    // dann wird beim naechsten Check das FS-Update geprueft
    dualOut.println("==============================");
    return;
  } else {
    logTS(); dualOut.println("[OTA] Firmware ist aktuell.");
  }
  
  // --- LittleFS-Update (nur wenn Firmware aktuell) ---
  if (remoteFsVersion.length() > 0 && remoteFsFile.length() > 0 && isVersionNewer(remoteFsVersion, localFsVersion)) {
    String fsUrl = String(update_bin_base_url) + remoteFsFile;
    logTS(); dualOut.println("[OTA] *** Neues LittleFS-Image gefunden! ***");
    logTS(); dualOut.print("[OTA] Lokal: ");
    dualOut.print(localFsVersion);
    dualOut.print(" -> Remote: ");
    dualOut.println(remoteFsVersion);
    logTS(); dualOut.print("[OTA] Download URL: ");
    dualOut.println(fsUrl);
    logTS(); dualOut.print("[OTA] Freier Heap vor FS-Update: ");
    dualOut.print(ESP.getFreeHeap());
    dualOut.println(" Bytes");
    logTS(); dualOut.println("[OTA] Starte LittleFS-Download und Flash...");
    
    // Log sichern bevor LittleFS ueberschrieben wird
    if (logFSready && logFile) {
      logFile.flush();
      logFile.close();
      logFSready = false;
    }
    
    // Hinweis: Log-Sicherung (getLogContent) und MQTT-Log-Versand hier ENTFERNT -
    // sie druecken den Heap vor dem TLS-Download zu stark (Download scheitert sonst).
    
    // MQTT vor dem TLS-Handshake trennen (Heap-Peak entlasten)
    if (mqttEnabled && mqttClient.connected()) {
      mqttClient.disconnect();
      logTS(); dualOut.println("[OTA] MQTT fuer LittleFS-Download getrennt");
    }
    delay(50);

    WiFiClient fsClient;
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    // Signaturpruefung auch fuer das FS-Image (laeuft beim Boot, ~18KB Heap,
    // daher unkritisch). Objekte transient -> im Normalbetrieb kein Verbrauch.
    BearSSL::PublicKey* fsPubKey = new BearSSL::PublicKey(ota_pubkey);
    BearSSL::HashSHA256* fsHash = new BearSSL::HashSHA256();
    BearSSL::SigningVerifier* fsVerifier = new BearSSL::SigningVerifier(fsPubKey);
    Update.installSignature(fsHash, fsVerifier);
    logTS(); dualOut.print("[OTA] FS-Signaturpruefung aktiv, freier Heap: ");
    dualOut.println(ESP.getFreeHeap());
    t_httpUpdate_return ret = ESPhttpUpdate.updateFS(fsClient, fsUrl);
    Update.installSignature(nullptr, nullptr);
    delete fsVerifier; delete fsHash; delete fsPubKey;
    
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        logTS(); dualOut.print("[OTA] FEHLER: LittleFS-Update fehlgeschlagen (");
        dualOut.print(ESPhttpUpdate.getLastError());
        dualOut.print("): ");
        dualOut.println(ESPhttpUpdate.getLastErrorString());
        // Log-Datei wieder oeffnen
        if (LittleFS.begin()) {
          logFile = LittleFS.open("/log.txt", "a");
          logFSready = (logFile);
        }
        break;
      case HTTP_UPDATE_NO_UPDATES:
        logTS(); dualOut.println("[OTA] Server meldet: Kein FS-Update verfuegbar.");
        break;
      case HTTP_UPDATE_OK:
        logTS(); dualOut.println("[OTA] LittleFS-Update erfolgreich! Neustart...");
        ESP.restart();
        break;
    }
  } else {
    logTS(); dualOut.println("[OTA] LittleFS ist aktuell.");
  }
  
  dualOut.println("==============================");
}
