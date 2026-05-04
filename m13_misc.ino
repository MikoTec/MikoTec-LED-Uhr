// m13_misc.ino

void clearEEPROM() {
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();
}


void clearssid() {
  EEPROM.begin(512);
  // write a 0 to ssid and pass bytes of the EEPROM
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();

}
void clearpass() {
  EEPROM.begin(512);
  // write a 0 to ssid and pass bytes of the EEPROM
  for (int i = 32; i < 96; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();

}


void loadFace(uint8_t partition)
{
  if (partition >= 0 && partition <= 4) { // only 3 locations for saved faces. Don't accidentally read/write other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color
    hourcolor.R = EEPROM.read(100 + partition * 15);
    hourcolor.G = EEPROM.read(101 + partition * 15);
    hourcolor.B = EEPROM.read(102 + partition * 15);

    //write the minute color
    minutecolor.R = EEPROM.read(103 + partition * 15);
    minutecolor.G = EEPROM.read(104 + partition * 15);
    minutecolor.B = EEPROM.read(105 + partition * 15);

    //write the blend point
    blendpoint = EEPROM.read(106 + partition * 15);

    //read the brightness
    uint8_t savedBrightness = EEPROM.read(107 + partition * 15);
    if (savedBrightness >= 10 && savedBrightness <= 100) {
      brightness = savedBrightness;
    }
  }
}
//-----------------------------Demo functions (for filming etc)---------------------------------

void webHandleNightModeDemo() {
  clockmode = normal;
  setTime(21, 59, 50, 1, 1, 1);
  sleep = 22;
  sleepmin = 0;
  server.send(200, "text/html", "demo of night mode");
}

void webHandleGame() {
  String toSend = FPSTR(game_html);
  if (webMode != 2) {
    // don't send external links if we're local only
    toSend.replace("$externallinks", FPSTR(externallinks));

  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");

  }
  toSend.replace("$playercolor", rgbToText(playercolors[nextplayer]));
  server.send(200, "html", toSend);
}

void handleHilfe() {
  logTS(); dualOut.println("Sending handleHilfe");
  if (LittleFS.exists("/hilfe.html")) {
    File f = LittleFS.open("/hilfe.html", "r");
    server.streamFile(f, "text/html"); f.close();
  } else {
    String toSend = FPSTR(hilfe_html);
    if (webMode != 2) toSend.replace("$externallinks", FPSTR(externallinks));
    else toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");
    toSend.replace("$menu", FPSTR(menu_html));
    server.send(200, "text/html", toSend);
  }
}

void handleSupport() {
  logTS(); dualOut.println("Sending handleSupport");
  if (LittleFS.exists("/support.html")) {
    File f = LittleFS.open("/support.html", "r");
    server.streamFile(f, "text/html"); f.close();
  } else {
    String toSend = FPSTR(support_html);
    if (webMode != 2) toSend.replace("$externallinks", FPSTR(externallinks));
    else toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");
    toSend.replace("$menu", FPSTR(menu_html));
    server.send(200, "text/html", toSend);
  }
}

void handleGetLog() {
  // ?prev=1 liefert den Log vom letzten Boot (log_prev.txt aus LittleFS)
  if (server.hasArg("prev") && server.arg("prev") == "1") {
    if (logFSready && LittleFS.exists("/log_prev.txt")) {
      File f = LittleFS.open("/log_prev.txt", "r");
      server.streamFile(f, "text/plain");
      f.close();
    } else {
      server.send(404, "text/plain", "Kein vorheriger Log vorhanden.");
    }
  } else {
    server.send(200, "text/plain", getLogContent());
  }
}

void handleGetSysInfo() {
  String json = "{";
  json += "\"fw\":\"" + String(firmware_version) + "\",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"up\":" + String(millis());
  json += "}";
  server.send(200, "application/json", json);
}

void handleReboot() {
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5;url=/'></head><body style='font-family:Abel,sans-serif;text-align:center;padding:40px'><h2>Neustart...</h2><p>Die Uhr startet neu. Du wirst in 5 Sekunden weitergeleitet.</p></body></html>");
  logTS(); dualOut.println("Reboot per Support-Seite ausgeloest");
  delay(500);
  ESP.reset();
}

void handleGetTime() {
  String t = timeToText((int)hour(), (int)minute());
  String s = (second() < 10) ? "0" + String(second()) : String(second());
  server.send(200, "text/plain", t + ":" + s);
}

void handleGetState() {
  String json = "{";
  json += "\"h\":" + String(hour()) + ",";
  json += "\"m\":" + String(minute()) + ",";
  json += "\"s\":" + String(second()) + ",";
  json += "\"clockmode\":" + String(clockmode) + ",";
  json += "\"showseconds\":" + String(showseconds) + ",";
  json += "\"showsunpoint\":" + String(showSunPoint) + ",";
  // Sonnenauf/-untergang in Minuten für JS-Uhr
  int gsM = month(), gsD = day(), gsDoy = gsD;
  int gsDIM[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  int gsY = year();
  if (gsY % 4 == 0 && (gsY % 100 != 0 || gsY % 400 == 0)) gsDIM[2] = 29;
  for (int i = 1; i < gsM; i++) gsDoy += gsDIM[i];
  float gsTz = timezone + DSTtime;
  int gsRH2, gsRM2, gsSH2, gsSM2;
  getSunTimes(gsDoy, latitude, longitude, gsTz, gsRH2, gsSH2);
  json += "\"sunriseMinutes\":" + String(gsRH2) + ",";
  json += "\"sunsetMinutes\":" + String(gsSH2) + ",";
  json += "\"hourmarks\":" + String(hourmarks) + ",";
  json += "\"pixelCount\":" + String(pixelCount) + ",";
  json += "\"brightness\":" + String(brightness) + ",";
  json += "\"hourR\":" + String(hourcolor.R) + ",";
  json += "\"hourG\":" + String(hourcolor.G) + ",";
  json += "\"hourB\":" + String(hourcolor.B) + ",";
  json += "\"minR\":" + String(minutecolor.R) + ",";
  json += "\"minG\":" + String(minutecolor.G) + ",";
  json += "\"minB\":" + String(minutecolor.B) + ",";
  json += "\"blendpoint\":" + String(blendpoint) + ",";
  json += "\"maxBrightness\":" + String((int)maxBrightness) + ",";
  json += "\"fw\":\"" + String(firmware_version) + "\",";
  json += "\"alarmactive\":" + String(clockmode == alarm ? 1 : 0) + ",";
  char hcHex[8], mcHex[8];
  snprintf(hcHex, sizeof(hcHex), "#%02x%02x%02x", hourcolor.R, hourcolor.G, hourcolor.B);
  snprintf(mcHex, sizeof(mcHex), "#%02x%02x%02x", minutecolor.R, minutecolor.G, minutecolor.B);
  json += "\"hourcolor\":\"" + String(hcHex) + "\",";
  json += "\"minutecolor\":\"" + String(mcHex) + "\",";
  // 4 Farbschemata aus EEPROM
  json += "\"schemes\":[";
  for (int i = 1; i <= 4; i++) {
    char sh[8], sm[8];
    snprintf(sh, sizeof(sh), "#%02x%02x%02x",
      EEPROM.read(100 + i*15), EEPROM.read(101 + i*15), EEPROM.read(102 + i*15));
    snprintf(sm, sizeof(sm), "#%02x%02x%02x",
      EEPROM.read(103 + i*15), EEPROM.read(104 + i*15), EEPROM.read(105 + i*15));
    int bp = EEPROM.read(106 + i*15);
    int br = EEPROM.read(107 + i*15);
    json += "{\"h\":\"" + String(sh) + "\",\"m\":\"" + String(sm) + "\",\"bp\":" + String(bp) + ",\"br\":" + String(br) + "}";
    if (i < 4) json += ",";
  }
  json += "]";
  json += "}";
  server.send(200, "application/json", json);
}

void handleGetSettings() {
  String json = "{";
  json += "\"pixelCount\":" + String(pixelCount) + ",";
  json += "\"maxBrightness\":" + String((int)maxBrightness) + ",";
  json += "\"clockname\":\"" + clockname + "\",";
  json += "\"hourmarks\":" + String(hourmarks) + ",";
  json += "\"sleeptype\":" + String(sleeptype) + ",";
  json += "\"sleep\":" + String(EEPROM.read(182)) + ",";
  json += "\"sleepmin\":" + String(EEPROM.read(183)) + ",";
  json += "\"wake\":" + String(EEPROM.read(189)) + ",";
  json += "\"wakemin\":" + String(EEPROM.read(190)) + ",";
  json += "\"nightbrightness\":" + String(int(nightBrightness)) + ",";
  json += "\"hemisphere\":" + String(hemisphere) + ",";
  json += "\"DSTauto\":" + String(DSTauto ? 1 : 0) + ",";
  json += "\"DSTtime\":" + String(DSTtime ? 1 : 0) + ",";
  json += "\"timezonevalue\":" + String(timezonevalue) + ",";
  json += "\"showseconds\":" + String(showseconds ? 1 : 0) + ",";
  json += "\"showsunpoint\":" + String(showSunPoint ? 1 : 0) + ",";
  json += "\"dawnbreak\":" + String(dawnbreak ? 1 : 0) + ",";
  json += "\"autosleep\":" + String(autoSleep ? 1 : 0) + ",";
  json += "\"latitude\":" + String(latitude) + ",";
  json += "\"longitude\":" + String(longitude) + ",";
  json += "\"timezone\":" + String(timezone) + ",";
  json += "\"mqttEnabled\":" + String(mqttEnabled ? 1 : 0) + ",";
  json += "\"mqttConnected\":" + String(mqttClient.connected() ? 1 : 0) + ",";
  json += "\"betaChannel\":" + String(betaChannel ? 1 : 0);
  json += "}";
  logTS(); dualOut.println("[SETTINGS] " + json);
  server.send(200, "application/json", json);
}

void webHandleTimeSet() {
  int timehr = 0, timemin = 0, timesec = 0;
  int timeday = 1, timemonth = 1, timeyear = 2024;

  if (server.hasArg("time")) {
    String timestring = server.arg("time");
    timehr  = timestring.substring(0, 2).toInt();
    timemin = timestring.substring(3, 5).toInt();
    if (timestring.length() >= 8) {
      timesec = timestring.substring(6, 8).toInt();
    }
  }
  if (server.hasArg("date")) {
    String datestring = server.arg("date");
    // Format: YYYY-MM-DD
    timeyear  = datestring.substring(0, 4).toInt();
    timemonth = datestring.substring(5, 7).toInt();
    timeday   = datestring.substring(8, 10).toInt();
  }

  setTime(timehr, timemin, timesec, timeday, timemonth, timeyear);
  updateTimestampCache();
  logTS(); dualOut.print("Zeit+Datum gesetzt: ");
  dualOut.print(timeday); dualOut.print(".");
  dualOut.print(timemonth); dualOut.print(".");
  dualOut.print(timeyear); dualOut.print(" ");
  dualOut.print(timehr); dualOut.print(":");
  dualOut.println(timemin);

  server.send(200, "text/plain", "OK");
}

void webHandleReflection() {
  if (testrun == 3) {
    testrun = 0;
    server.send(200, "text", "Clock has been set to normal mode.");
  }
  else {
    testrun = 3;
    server.send(200, "text", "Clock has been set to reflection mode.");
  }
}

void webHandleDawn() {
  dawntest();
  server.send(200, "text", "test dawn");
}

void webHandleMoon() {
  moontest();
  server.send(200, "text", "test moon");
}

void webHandleAlarm() {
  String toSend = FPSTR(alarm_html);
  if (webMode != 2) {
    // don't send external links if we're local only
    toSend.replace("$externallinks", FPSTR(externallinks));

  } else {
    toSend.replace("$externallinks", "<link rel=stylesheet href='clockmenustyle.css'>");

  }
  server.send(200, "html", toSend);

}


//------------------------------NTP Functions---------------------------------


time_t getNTPtime(void)
{
  time_t newtime;
  NTPclient.forceUpdate();
  newtime = NTPclient.getEpochTime();
  logTS(); dualOut.print("NTP Zeit: ");
  dualOut.println(newtime);
  for (int i = 0; i < 5; i++) {
    if (newtime == 0) {
      logTS(); dualOut.println("Failed NTP Attempt");
      delay(2000);
      NTPclient.forceUpdate();
      newtime = NTPclient.getEpochTime();
    }
  }

  return newtime;
}

//---------------------------------------SSDP repsponding fucntions-------------------------------------------------------

void ssdpResponder() {
  //WiFiClient client = HTTP.client();
  int clockname_len = clockname.length() + 1;
  char clocknamechar[clockname_len];
  clockname.toCharArray(clocknamechar, clockname_len);
  String str = "<root><specVersion><major>1</major><minor>0</minor></specVersion><URLBase>http://" + ipString + ":80/</URLBase><device><deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType><friendlyName>" + clocknamechar + "(" + ipString + ")</friendlyName><manufacturer>MikoTec</manufacturer><manufacturerURL>http://www.mikotec-led-uhr.de</manufacturerURL><modelDescription>MikoTec LED Uhr v1</modelDescription><modelName>MikoTec LED Uhr v1</modelName><modelNumber>4</modelNumber><modelURL>http://www.mikotec-led-uhr.de</modelURL><serialNumber>3</serialNumber><UDN>uuid:3</UDN><presentationURL>index.html</presentationURL></device></root>";
  server.send(200, "text/plain", str);
}

String StringIPaddress(IPAddress myaddr)
{
  String LocalIP = "";
  for (int i = 0; i < 4; i++)
  {
    LocalIP += String(myaddr[i]);
    if (i < 3) LocalIP += ".";
  }
  return LocalIP;
}
//----------------------------------------DST adjusting functions------------------------------------------------------------------
void interpretTimeZone(int timezonename) {
  switch (timezonename) {
    case 1: timezone = -12; break;
    case 2: timezone = -11; break;
    case 3: timezone = -10; break;
    case 4: timezone = -9; break;
    case 5: timezone = -8; break;
    case 6: timezone = -8; break;
    case 7: timezone = -7; break;
    case 8: timezone = -7; break;
    case 9: timezone = -7; break;
    case 10: timezone = -6; break;
    case 11: timezone = -6; break;
    case 12: timezone = -6; break;
    case 13: timezone = -6; break;
    case 14: timezone = -5; break;
    case 15: timezone = -5; break;
    case 16: timezone = -5; break;
    case 17: timezone = -4; break;
    case 18: timezone = -4; break;
    case 19: timezone = -4; break;
    case 20: timezone = -4; break;
    case 21: timezone = -3.5; break;
    case 22: timezone = -3; break;
    case 23: timezone = -3; break;
    case 24: timezone = -3; break;
    case 25: timezone = -3; break;
    case 26: timezone = -2; break;
    case 27: timezone = -1; break;
    case 28: timezone = -1; break;
    case 29: timezone = 0; break;
    case 30: timezone = 0; break;
    case 31: timezone = 1; break;
    case 32: timezone = 1; break;
    case 33: timezone = 1; break;
    case 34: timezone = 1; break;
    case 35: timezone = 1; break;
    case 36: timezone = 2; break;
    case 37: timezone = 2; break;
    case 38: timezone = 2; break;
    case 39: timezone = 2; break;
    case 40: timezone = 2; break;
    case 41: timezone = 2; break;
    case 42: timezone = 2; break;
    case 43: timezone = 2; break;
    case 44: timezone = 2; break;
    case 45: timezone = 3; break;
    case 46: timezone = 3; break;
    case 47: timezone = 3; break;
    case 48: timezone = 3; break;
    case 49: timezone = 3.5; break;
    case 50: timezone = 4; break;
    case 51: timezone = 4; break;
    case 52: timezone = 4; break;
    case 53: timezone = 4.5; break;
    case 54: timezone = 5; break;
    case 55: timezone = 5; break;
    case 56: timezone = 5.5; break;
    case 57: timezone = 5.5; break;
    case 58: timezone = 5.75; break;
    case 59: timezone = 6; break;
    case 60: timezone = 6; break;
    case 61: timezone = 6.5; break;
    case 62: timezone = 7; break;
    case 63: timezone = 7; break;
    case 64: timezone = 8; break;
    case 65: timezone = 8; break;
    case 66: timezone = 8; break;
    case 67: timezone = 8; break;
    case 68: timezone = 8; break;
    case 69: timezone = 9; break;
    case 70: timezone = 9; break;
    case 71: timezone = 9; break;
    case 72: timezone = 9.5; break;
    case 73: timezone = 9.5; break;
    case 74: timezone = 10; break;
    case 75: timezone = 10; break;
    case 76: timezone = 10; break;
    case 77: timezone = 10; break;
    case 78: timezone = 10; break;
    case 79: timezone = 11; break;
    case 80: timezone = 12; break;
    case 81: timezone = 12; break;
    case 82: timezone = 13; break;
  }
}
void ChangeNeoPixels(uint16_t count, uint8_t pin)
{
  if (clockleds)
  {
    delete clockleds;
  }
  clockleds = new NeoPixelBusType(count); // UART1 nutzt immer GPIO2 (D4)
  clockleds->Begin();
}
//-----------------------------------------------------------------------------------------------------websocket stuff------------------------------------------------------------------------------------------------------

String wsHead(String input){
  int headend = find_text("|", input);
  return input.substring(0,headend);
}

String wsValue(String input){
  int valuestart = find_text("|", input)+1;
  return input.substring(valuestart);
}

int find_text(String needle, String haystack) {
  int foundpos = -1;
  for (int i = 0; i <= haystack.length() - needle.length(); i++) {
    if (haystack.substring(i,needle.length()+i) == needle) {
      foundpos = i;
    }
  }
  return foundpos;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

 switch (type) {
   case WStype_DISCONNECTED:
     dualOut.printf("[%u] Disconnected!\n", num);
     break;
   case WStype_CONNECTED: {
       IPAddress ip = webSocket.remoteIP(num);
       dualOut.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

       // send message to client
       webSocket.sendTXT(num, "Connected");
     }
     break;
   case WStype_TEXT:

     String value = wsValue((char*)payload);
     String head = wsHead((char*)payload);

     if(head=="hourcolor"){
        getRGB(value, hourcolor);
     }
     if(head=="minutecolor"){
        getRGB(value, minutecolor);
     }
     if(head=="brightness"){
       brightness = (int)value.toInt();
     }
     if(head=="blendpoint"){
       blendpoint = (uint8_t)value.toInt();
     }
     if(head=="newplayer"){

       gamejoin(num);
     }
     if(head=="gamestart"){
       gamestart();
     }
     if(head=="gameplus"){
       gameplus(num);
     }


     break;
 }

}


//========================================GAME FUNCTIONS======================================================================

void gamestart(){
  logTS(); dualOut.println("start command received");
  gamestartticker.attach_ms(50, gamecountdown);

}

void gamecountdown(){
  logTS(); dualOut.print("Gamebrightness: ");
  dualOut.println(gamebrightness);
  gamebrightness = gamebrightness - (maxBrightness/50);
  if (gamebrightness<=0) {
    gamestarted = 1;
    gamestartticker.detach();
    gamebrightness = maxBrightness;
  }
}

void gamejoin(int num){
  if (gamestarted == 0) {
    gamebrightness = maxBrightness;
    nextplayer++;
    gamearray[num] = gamestartpoints;
    clockmode = game;
    playercount = 0;
    for(int i=0; i<6; i++){
      if(gamearray[i]==gamestartpoints){
        playercount++;
      }
    }
  }
}
void gameplus(int playernum){
  if(gamestarted==1){
    if(gamearray[playernum]>0){//if your score is 0 or less then you're eliminated
      for(int i=0; i<playercount; i++){
        if(playernum == i){
          gamearray[i] += (playersremaining-1);//add to the clicking players score a point for each opponant
        } else {
          gamearray[i]--;//take that point off everyone else
          gamearray[i] = _max(gamearray[i],0);
        }
      }
    }

    playersremaining = 0;//check if we have a winner
    int winner = 0;
    for(int i=0; i<playercount; i++){
      if(gamearray[i]>0){
        winner = i;
        playersremaining++;
      }
    }
    if(playersremaining <= 1){
      animatewinner(winner);
    }

    //debug
    int accumulatedscore=gamearray[0];
    int totalpoints=playercount*gamestartpoints;
    for (size_t i = 0; i < playercount; i++) {

      logTS(); dualOut.print("Player ");
      dualOut.print(i);
      logTS(); dualOut.print(" score: ");
      dualOut.print(gamearray[i]);
      logTS(); dualOut.print(" animate to: ");
      dualOut.println((int)((float)accumulatedscore/(float)totalpoints*pixelCount));
      accumulatedscore+=gamearray[i+1];
    }
  }

}

void animatewinner(int winner){
  for (size_t i = 0; i < 6; i++) {
    gamearray[i]=0;
    gamestarted=0;
    nextplayer=0;
  }
  for (size_t i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, playercolors[winner]);
  }

  delay(1000);
  nightCheck();
}
void gameface(){
int playeranimating = 0;
int accumulatedscore = gamearray[0];
int totalpoints = playercount*gamestartpoints;
  if(gamestarted==0){
    for (size_t i = 0; i < pixelCount; i++) {
      if(i < ((playeranimating + 1) * pixelCount/playercount)){
        clockleds->SetPixelColor(i, RgbColor::LinearBlend(RgbColor(0,0,0), playercolors[playeranimating], (float)gamebrightness/255.0f));
      } else {
        playeranimating++;
      }
    }
  } else {
    for (size_t i = 0; i < pixelCount; i++) {
      if(i < ((float)accumulatedscore/(float)totalpoints*pixelCount)){
        clockleds->SetPixelColor(i, playercolors[playeranimating]);
      } else {
        playeranimating++;
        accumulatedscore += gamearray[playeranimating];
      }
    }
  }

}
