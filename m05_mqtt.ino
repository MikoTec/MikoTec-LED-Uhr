// m05_mqtt.ino

String mqttBaseTopic() {
  return "lightclock/" + clockname;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  logTS(); dualOut.println("[MQTT] Empfangen: " + t + " = " + msg);

  String base = mqttBaseTopic() + "/set/";

  if (t == base + "brightness") {
    int val = msg.toInt();
    if (val >= 0 && val <= 100) {
      brightness = _max(10, val);
      EEPROM.begin(512);
      EEPROM.write(191, brightness);
      EEPROM.commit();
      logTS(); dualOut.println("[MQTT] Helligkeit: " + String(brightness));
    }
  }
  else if (t == base + "hourcolor") {
    msg.replace("%23", "#");
    getRGB(msg, hourcolor);
    EEPROM.begin(512);
    EEPROM.write(100, hourcolor.R);
    EEPROM.write(101, hourcolor.G);
    EEPROM.write(102, hourcolor.B);
    EEPROM.commit();
    logTS(); dualOut.println("[MQTT] Stundenfarbe: " + msg);
  }
  else if (t == base + "minutecolor") {
    msg.replace("%23", "#");
    getRGB(msg, minutecolor);
    EEPROM.begin(512);
    EEPROM.write(103, minutecolor.R);
    EEPROM.write(104, minutecolor.G);
    EEPROM.write(105, minutecolor.B);
    EEPROM.commit();
    logTS(); dualOut.println("[MQTT] Minutenfarbe: " + msg);
  }
  else if (t == base + "blendpoint") {
    int val = msg.toInt();
    if (val >= 0 && val <= 100) {
      blendpoint = val;
      EEPROM.begin(512);
      EEPROM.write(106, blendpoint);
      EEPROM.commit();
      logTS(); dualOut.println("[MQTT] Blendpoint: " + String(blendpoint));
    }
  }
  else if (t == base + "hourmarks") {
    int val = -1;
    if (msg == "Keine") val = 0;
    else if (msg == "Mittag") val = 1;
    else if (msg == "Quadranten") val = 2;
    else if (msg == "Stunden") val = 3;
    else if (msg == "Abdunkeln") val = 4;
    else val = msg.toInt();
    if (val >= 0 && val <= 4) {
      hourmarks = val;
      EEPROM.begin(512);
      EEPROM.write(181, hourmarks);
      EEPROM.commit();
      logTS(); dualOut.println("[MQTT] Stundenmarken: " + String(hourmarks));
    }
  }
  else if (t == base + "showseconds") {
    showseconds = (msg == "1" || msg == "true" || msg == "ON");
    EEPROM.begin(512);
    EEPROM.write(184, showseconds);
    EEPROM.commit();
    logTS(); dualOut.println("[MQTT] Sekunden: " + String(showseconds));
  }
  else if (t == base + "showsunpoint") {
    showSunPoint = (msg == "1" || msg == "true" || msg == "ON");
    EEPROM.begin(512);
    EEPROM.write(235, showSunPoint);
    EEPROM.commit();
    logTS(); dualOut.println("[MQTT] Sonnenpunkt: " + String(showSunPoint));
  }
  else if (t == base + "clockmode") {
    if (msg == "normal") clockmode = normal;
    else if (msg == "night") clockmode = night;
    else if (msg == "dawn") { clockmode = dawnmode; dawnprogress = 0; dawntick.attach(14, dawnadvance); }
    logTS(); dualOut.println("[MQTT] Clockmode: " + msg);
  }
  else if (t == base + "power") {
    if (msg == "ON") {
      // Vorherigen Zustand wiederherstellen
      if (mqttPrevClockmode >= 0) {
        clockmode = mqttPrevClockmode;
        sleeptype = mqttPrevSleeptype;
        mqttPrevClockmode = -1;
        mqttPrevSleeptype = -1;
      } else {
        clockmode = normal;
      }
      logTS(); dualOut.println("[MQTT] Power ON");
    } else if (msg == "OFF") {
      // Zustand merken, dann komplett ausschalten
      if (clockmode != night || sleeptype != black) {
        mqttPrevClockmode = clockmode;
        mqttPrevSleeptype = sleeptype;
      }
      clockmode = night;
      sleeptype = black;
      logTS(); dualOut.println("[MQTT] Power OFF");
    }
  }
  else if (t == base + "beta") {
    betaChannel = (msg == "1" || msg == "true" || msg == "ON");
    EEPROM.begin(512);
    EEPROM.write(367, betaChannel ? 1 : 0);
    EEPROM.commit();
    logTS(); dualOut.println("[MQTT] Beta-Channel: " + String(betaChannel ? "Ja" : "Nein"));
  }
  else if (t == base + "update") {
    if (msg == "install" || msg == "INSTALL" || msg == "PRESS") {
      logTS(); dualOut.println("[MQTT] Update per MQTT ausgeloest");
      mqttPublishState();
      checkForUpdate();
      return; // checkForUpdate kann reboot ausloesen
    } else if (msg == "check") {
      logTS(); dualOut.println("[MQTT] Update-Check per MQTT ausgeloest");
      checkForUpdate();
    }
  }

  // Nach jeder Aenderung sofort State publishen
  mqttPublishState();
}

void mqttPublishUpdateState() {
  if (!mqttClient.connected()) return;
  String base = mqttBaseTopic();
  String json = "{";
  json += "\"installed_version\":\"" + String(firmware_version) + "\",";
  if (mqttAvailableVersion.length() > 0 && isNewerVersion(mqttAvailableVersion)) {
    json += "\"latest_version\":\"" + mqttAvailableVersion + "\",";
    json += "\"title\":\"MikoTec LED Uhr\",";
    json += "\"release_url\":\"https://github.com/MikoTec/MikoTec-LED-Uhr\"";
  } else {
    json += "\"latest_version\":\"" + String(firmware_version) + "\"";
  }
  json += "}";
  mqttClient.publish((base + "/update_state").c_str(), json.c_str(), true);
}

void mqttPublishState() {
  if (!mqttClient.connected()) return;

  String base = mqttBaseTopic();
  char hcHex[8], mcHex[8];
  snprintf(hcHex, sizeof(hcHex), "#%02x%02x%02x", hourcolor.R, hourcolor.G, hourcolor.B);
  snprintf(mcHex, sizeof(mcHex), "#%02x%02x%02x", minutecolor.R, minutecolor.G, minutecolor.B);

  // State JSON fuer Home Assistant
  String cmName = "normal";
  if (clockmode == night) cmName = "night";
  else if (clockmode == dawnmode) cmName = "dawn";

  String json = "{";
  json += "\"state\":\"" + String(clockmode != night ? "ON" : "OFF") + "\",";
  json += "\"brightness\":" + String(map(brightness, 0, 100, 0, 255)) + ",";
  json += "\"brightness_pct\":" + String(brightness) + ",";
  json += "\"color\":{\"r\":" + String(hourcolor.R) + ",\"g\":" + String(hourcolor.G) + ",\"b\":" + String(hourcolor.B) + "},";
  json += "\"clockmode\":" + String(clockmode) + ",";
  json += "\"clockmode_name\":\"" + cmName + "\",";
  json += "\"showseconds\":" + String(showseconds ? 1 : 0) + ",";
  json += "\"showsunpoint\":" + String(showSunPoint ? 1 : 0) + ",";
  json += "\"hourmarks\":" + String(hourmarks) + ",";
  String hmName = "Keine";
  if (hourmarks == 1) hmName = "Mittag";
  else if (hourmarks == 2) hmName = "Quadranten";
  else if (hourmarks == 3) hmName = "Stunden";
  else if (hourmarks == 4) hmName = "Abdunkeln";
  json += "\"hourmarks_name\":\"" + hmName + "\",";
  json += "\"blendpoint\":" + String(blendpoint) + ",";
  json += "\"hourcolor\":\"" + String(hcHex) + "\",";
  json += "\"minutecolor\":\"" + String(mcHex) + "\",";
  json += "\"fw\":\"" + String(firmware_version) + "\",";
  json += "\"beta\":" + String(betaChannel ? 1 : 0) + ",";

  // Sonnenauf/untergang
  int srMin = 0, ssMin = 0;
  time_t rawtime = now();
  struct tm *ti = localtime(&rawtime);
  int doy = ti->tm_yday + 1;
  getSunTimes(doy, latitude, longitude, timezone + DSTtime, srMin, ssMin);
  char srBuf[6], ssBuf[6];
  snprintf(srBuf, sizeof(srBuf), "%02d:%02d", srMin/60, srMin%60);
  snprintf(ssBuf, sizeof(ssBuf), "%02d:%02d", ssMin/60, ssMin%60);
  json += "\"sunrise\":\"" + String(srBuf) + "\",";
  json += "\"sunset\":\"" + String(ssBuf) + "\"";
  json += "}";

  mqttClient.publish((base + "/state").c_str(), json.c_str(), true);
}

void mqttPublishDiscovery() {
  if (!mqttClient.connected()) return;

  String uid = "mikotec_" + clockname;
  uid.replace("-", "_");
  String base = mqttBaseTopic();

  // Device-Block (wird in jeder Entity wiederverwendet)
  String dev = "\"dev\":{";
  dev += "\"ids\":[\"" + uid + "\"],";
  dev += "\"name\":\"MikoTec LED Uhr\",";
  dev += "\"mdl\":\"Light Clock\",";
  dev += "\"mf\":\"MikoTec\",";
  dev += "\"sw\":\"" + String(firmware_version) + "\"";
  dev += "}";

  // Availability-Block
  String avail = "\"avty_t\":\"" + base + "/availability\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\"";

  // --- 1) Switch: Power (ON/OFF) ---
  {
    String topic = "homeassistant/switch/" + uid + "_power/config";
    String json = "{";
    json += "\"name\":\"Power\",";
    json += "\"uniq_id\":\"" + uid + "_power\",";
    json += "\"cmd_t\":\"" + base + "/set/power\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.state }}\",";
    json += "\"ic\":\"mdi:clock-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 2) Number: Helligkeit (0-100) ---
  {
    String topic = "homeassistant/number/" + uid + "_brightness/config";
    String json = "{";
    json += "\"name\":\"Helligkeit\",";
    json += "\"uniq_id\":\"" + uid + "_brightness\",";
    json += "\"cmd_t\":\"" + base + "/set/brightness\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.brightness_pct }}\",";
    json += "\"min\":10,\"max\":100,\"step\":1,";
    json += "\"unit_of_meas\":\"%\",";
    json += "\"ic\":\"mdi:brightness-6\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 3) Select: Clockmode ---
  {
    String topic = "homeassistant/select/" + uid + "_clockmode/config";
    String json = "{";
    json += "\"name\":\"Modus\",";
    json += "\"uniq_id\":\"" + uid + "_clockmode\",";
    json += "\"cmd_t\":\"" + base + "/set/clockmode\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.clockmode_name }}\",";
    json += "\"ops\":[\"normal\",\"night\",\"dawn\"],";
    json += "\"ic\":\"mdi:theme-light-dark\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 4) Switch: Sekunden ---
  {
    String topic = "homeassistant/switch/" + uid + "_seconds/config";
    String json = "{";
    json += "\"name\":\"Sekunden\",";
    json += "\"uniq_id\":\"" + uid + "_seconds\",";
    json += "\"cmd_t\":\"" + base + "/set/showseconds\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ 'ON' if value_json.showseconds == 1 else 'OFF' }}\",";
    json += "\"ic\":\"mdi:timer-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 5) Switch: Sonnenpunkt ---
  {
    String topic = "homeassistant/switch/" + uid + "_sunpoint/config";
    String json = "{";
    json += "\"name\":\"Sonnenpunkt\",";
    json += "\"uniq_id\":\"" + uid + "_sunpoint\",";
    json += "\"cmd_t\":\"" + base + "/set/showsunpoint\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ 'ON' if value_json.showsunpoint == 1 else 'OFF' }}\",";
    json += "\"ic\":\"mdi:weather-sunny\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 6) Select: Stundenmarken ---
  {
    String topic = "homeassistant/select/" + uid + "_hourmarks/config";
    String json = "{";
    json += "\"name\":\"Stundenmarken\",";
    json += "\"uniq_id\":\"" + uid + "_hourmarks\",";
    json += "\"cmd_t\":\"" + base + "/set/hourmarks\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.hourmarks_name }}\",";
    json += "\"ops\":[\"Keine\",\"Mittag\",\"Quadranten\",\"Stunden\",\"Abdunkeln\"],";
    json += "\"ic\":\"mdi:clock-digital\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 7) Number: Blendpoint ---
  {
    String topic = "homeassistant/number/" + uid + "_blendpoint/config";
    String json = "{";
    json += "\"name\":\"Blendpoint\",";
    json += "\"uniq_id\":\"" + uid + "_blendpoint\",";
    json += "\"cmd_t\":\"" + base + "/set/blendpoint\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.blendpoint }}\",";
    json += "\"min\":0,\"max\":100,\"step\":1,";
    json += "\"unit_of_meas\":\"%\",";
    json += "\"ic\":\"mdi:gradient-horizontal\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 8) Sensor: Firmware ---
  {
    String topic = "homeassistant/sensor/" + uid + "_firmware/config";
    String json = "{";
    json += "\"name\":\"Firmware\",";
    json += "\"uniq_id\":\"" + uid + "_firmware\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.fw }}\",";
    json += "\"ic\":\"mdi:information-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 9) Text: Stundenfarbe ---
  {
    String topic = "homeassistant/text/" + uid + "_hourcolor/config";
    String json = "{";
    json += "\"name\":\"Stundenfarbe\",";
    json += "\"uniq_id\":\"" + uid + "_hourcolor\",";
    json += "\"cmd_t\":\"" + base + "/set/hourcolor\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.hourcolor }}\",";
    json += "\"min\":7,\"max\":7,";
    json += "\"ic\":\"mdi:palette\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 10) Text: Minutenfarbe ---
  {
    String topic = "homeassistant/text/" + uid + "_minutecolor/config";
    String json = "{";
    json += "\"name\":\"Minutenfarbe\",";
    json += "\"uniq_id\":\"" + uid + "_minutecolor\",";
    json += "\"cmd_t\":\"" + base + "/set/minutecolor\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.minutecolor }}\",";
    json += "\"min\":7,\"max\":7,";
    json += "\"ic\":\"mdi:palette-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 11) Sensor: Log ---
  {
    String topic = "homeassistant/sensor/" + uid + "_log/config";
    String json = "{";
    json += "\"name\":\"Log\",";
    json += "\"uniq_id\":\"" + uid + "_log\",";
    json += "\"stat_t\":\"" + base + "/log\",";
    json += "\"ic\":\"mdi:text-box-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 12) Sensor: Sonnenaufgang ---
  {
    String topic = "homeassistant/sensor/" + uid + "_sunrise/config";
    String json = "{";
    json += "\"name\":\"Sonnenaufgang\",";
    json += "\"uniq_id\":\"" + uid + "_sunrise\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.sunrise }}\",";
    json += "\"ic\":\"mdi:weather-sunset-up\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 13) Sensor: Sonnenuntergang ---
  {
    String topic = "homeassistant/sensor/" + uid + "_sunset/config";
    String json = "{";
    json += "\"name\":\"Sonnenuntergang\",";
    json += "\"uniq_id\":\"" + uid + "_sunset\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ value_json.sunset }}\",";
    json += "\"ic\":\"mdi:weather-sunset-down\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 14) Switch: Beta-Channel ---
  {
    String topic = "homeassistant/switch/" + uid + "_beta/config";
    String json = "{";
    json += "\"name\":\"Beta-Updates\",";
    json += "\"uniq_id\":\"" + uid + "_beta\",";
    json += "\"cmd_t\":\"" + base + "/set/beta\",";
    json += "\"stat_t\":\"" + base + "/state\",";
    json += "\"val_tpl\":\"{{ 'ON' if value_json.beta == 1 else 'OFF' }}\",";
    json += "\"ic\":\"mdi:flask-outline\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }

  // --- 13) Update: Firmware ---
  {
    String topic = "homeassistant/update/" + uid + "_firmware_update/config";
    String json = "{";
    json += "\"name\":\"Firmware Update\",";
    json += "\"uniq_id\":\"" + uid + "_firmware_update\",";
    json += "\"stat_t\":\"" + base + "/update_state\",";
    json += "\"cmd_t\":\"" + base + "/set/update\",";
    json += "\"payload_install\":\"install\",";
    json += "\"latest_version_topic\":\"" + base + "/update_state\",";
    json += "\"latest_version_template\":\"{{ value_json.latest_version }}\",";
    json += "\"entity_picture\":\"https://brands.home-assistant.io/_/mqtt/icon.png\",";
    json += "\"ic\":\"mdi:update\",";
    json += avail + "," + dev + "}";
    mqttClient.publish(topic.c_str(), json.c_str(), true);
    logTS(); dualOut.println("[MQTT] Discovery: " + topic);
  }
}

bool mqttReconnect() {
  String clientId = "lightclock-" + clockname;
  bool connected;
  if (strlen(mqttUser) > 0) {
    connected = mqttClient.connect(clientId.c_str(), mqttUser, mqttPass,
                 (mqttBaseTopic() + "/availability").c_str(), 0, true, "offline");
  } else {
    connected = mqttClient.connect(clientId.c_str(),
                 (mqttBaseTopic() + "/availability").c_str(), 0, true, "offline");
  }
  if (connected) {
    logTS(); dualOut.println("[MQTT] Verbunden mit " + String(mqttBroker));
    // Availability publishen
    mqttClient.publish((mqttBaseTopic() + "/availability").c_str(), "online", true);
    // Alle set-Topics subscriben
    String base = mqttBaseTopic() + "/set/#";
    mqttClient.subscribe(base.c_str());
    logTS(); dualOut.println("[MQTT] Subscribed: " + base);
    // Discovery senden
    if (!mqttDiscoverySent) {
      // Alte Light-Discovery entfernen
      String uid = "mikotec_" + clockname;
      uid.replace("-", "_");
      String oldTopic = "homeassistant/light/" + uid + "/config";
      mqttClient.publish(oldTopic.c_str(), "", true);
      // Alte Sensor-Discoveries entfernen (jetzt Text statt Sensor)
      mqttClient.publish(("homeassistant/sensor/" + uid + "_hourcolor/config").c_str(), "", true);
      mqttClient.publish(("homeassistant/sensor/" + uid + "_minutecolor/config").c_str(), "", true);
      logTS(); dualOut.println("[MQTT] Alte Discoveries entfernt");

      mqttPublishDiscovery();
      mqttDiscoverySent = true;
    }
    // Initialen State publishen
    mqttPublishState();
    mqttPublishUpdateState();
    return true;
  } else {
    logTS(); dualOut.println("[MQTT] Verbindung fehlgeschlagen, rc=" + String(mqttClient.state()));
    return false;
  }
}

void loadMqttConfig() {
  EEPROM.begin(512);
  mqttEnabled = EEPROM.read(236);
  if (mqttEnabled > 1) mqttEnabled = false;
  mqttPort = (EEPROM.read(237) << 8) | EEPROM.read(238);
  if (mqttPort == 0 || mqttPort == 65535) mqttPort = 1883;
  // Broker (64 bytes ab 239)
  for (int i = 0; i < 63; i++) {
    mqttBroker[i] = char(EEPROM.read(239 + i));
  }
  mqttBroker[63] = '\0';
  // Abschneiden bei erstem Nullbyte
  for (int i = 0; i < 63; i++) {
    if (mqttBroker[i] == '\0' || mqttBroker[i] == 255) { mqttBroker[i] = '\0'; break; }
  }
  // User (32 bytes ab 303)
  for (int i = 0; i < 31; i++) {
    mqttUser[i] = char(EEPROM.read(303 + i));
  }
  mqttUser[31] = '\0';
  for (int i = 0; i < 31; i++) {
    if (mqttUser[i] == '\0' || mqttUser[i] == 255) { mqttUser[i] = '\0'; break; }
  }
  // Pass (32 bytes ab 335)
  for (int i = 0; i < 31; i++) {
    mqttPass[i] = char(EEPROM.read(335 + i));
  }
  mqttPass[31] = '\0';
  for (int i = 0; i < 31; i++) {
    if (mqttPass[i] == '\0' || mqttPass[i] == 255) { mqttPass[i] = '\0'; break; }
  }

  logTS(); dualOut.println("[MQTT] Config geladen:");
  logTS(); dualOut.println("  Enabled: " + String(mqttEnabled));
  logTS(); dualOut.println("  Broker: " + String(mqttBroker));
  logTS(); dualOut.println("  Port: " + String(mqttPort));
  logTS(); dualOut.println("  User: " + String(mqttUser));
}

void saveMqttConfig() {
  EEPROM.begin(512);
  EEPROM.write(236, mqttEnabled ? 1 : 0);
  EEPROM.write(237, (mqttPort >> 8) & 0xFF);
  EEPROM.write(238, mqttPort & 0xFF);
  // Broker
  for (int i = 0; i < 64; i++) {
    EEPROM.write(239 + i, (i < (int)strlen(mqttBroker)) ? mqttBroker[i] : 0);
  }
  // User
  for (int i = 0; i < 32; i++) {
    EEPROM.write(303 + i, (i < (int)strlen(mqttUser)) ? mqttUser[i] : 0);
  }
  // Pass
  for (int i = 0; i < 32; i++) {
    EEPROM.write(335 + i, (i < (int)strlen(mqttPass)) ? mqttPass[i] : 0);
  }
  EEPROM.commit();
  logTS(); dualOut.println("[MQTT] Config gespeichert");
}

void handleGetMqtt() {
  String json = "{";
  json += "\"enabled\":" + String(mqttEnabled ? 1 : 0) + ",";
  json += "\"broker\":\"" + String(mqttBroker) + "\",";
  json += "\"port\":" + String(mqttPort) + ",";
  json += "\"user\":\"" + String(mqttUser) + "\",";
  json += "\"connected\":" + String(mqttClient.connected() ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetMqtt() {
  if (server.hasArg("mqtt_enabled")) {
    mqttEnabled = (server.arg("mqtt_enabled") == "1");
  }
  if (server.hasArg("mqtt_broker")) {
    String b = server.arg("mqtt_broker");
    b.trim();
    strncpy(mqttBroker, b.c_str(), 63);
    mqttBroker[63] = '\0';
  }
  if (server.hasArg("mqtt_port")) {
    mqttPort = server.arg("mqtt_port").toInt();
    if (mqttPort == 0) mqttPort = 1883;
  }
  if (server.hasArg("mqtt_user")) {
    String u = server.arg("mqtt_user");
    u.trim();
    strncpy(mqttUser, u.c_str(), 31);
    mqttUser[31] = '\0';
  }
  if (server.hasArg("mqtt_pass")) {
    String p = server.arg("mqtt_pass");
    strncpy(mqttPass, p.c_str(), 31);
    mqttPass[31] = '\0';
  }
  saveMqttConfig();

  // MQTT neu konfigurieren
  mqttClient.disconnect();
  mqttDiscoverySent = false;
  if (mqttEnabled && strlen(mqttBroker) > 0) {
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttReconnect();
  }

  server.send(200, "application/json", "{\"ok\":1}");
}
