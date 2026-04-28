#include <Arduino.h>



/*This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <WebSocketsServer.h>
#include <math.h>
#include <TimeLib.h>
//#include <TimeAlarms.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>
//#include <ntp.h>
#include <Ticker.h>
//#include <FastLED.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "h/settings.h"
#include "h/root.h"
#include "h/menu.h"
#include "h/timezone.h"
#include "h/timezonesetup.h"
#include "h/css.h"
#include "h/webconfig.h"
#include "h/importfonts.h"
#include "h/clearromsure.h"
#include "h/password.h"
#include "h/buttongradient.h"
#include "h/externallinks.h"
#include "h/spectrumcss.h"
#include "h/send_progmem.h"
#include "h/colourjs.h"
#include "h/clockjs.h"
#include "h/spectrumjs.h"
#include "h/alarm.h"
#include "h/game.h"
#include "h/hilfe.h"
#include "h/support.h"
#include <ESP8266HTTPUpdateServer.h>
#include <LittleFS.h>

#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
// Hier nur ganz einfach ohne die Berechnungen:
NTPClient NTPclient(ntpUDP, "pool.ntp.org");

#ifndef _max
#define _max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef _min
#define _min(a,b) ((a)<(b)?(a):(b))
#endif

// Serial-Ringpuffer fuer Support-Seite
#define LOG_BUFFER_SIZE 6144
char logBuffer[LOG_BUFFER_SIZE];
int logWritePos = 0;
bool logWrapped = false;
bool logFSready = false;  // true sobald LittleFS bereit und Log-Datei offen
File logFile;             // globales Handle fuer /log.txt in LittleFS

void logAppend(const char* str) {
  const char* p = str;
  while (*p) {
    logBuffer[logWritePos] = *p;
    logWritePos++;
    if (logWritePos >= LOG_BUFFER_SIZE) {
      logWritePos = 0;
      logWrapped = true;
    }
    p++;
  }
  // Zusaetzlich in LittleFS persistieren (kein flush - zu langsam)
  if (logFSready && logFile) {
    logFile.print(str);
  }
}

String getLogContent() {
  String result;
  result.reserve(LOG_BUFFER_SIZE);
  if (logWrapped) {
    for (int i = logWritePos; i < LOG_BUFFER_SIZE; i++) {
      if (logBuffer[i] != 0) result += logBuffer[i];
    }
  }
  for (int i = 0; i < logWritePos; i++) {
    if (logBuffer[i] != 0) result += logBuffer[i];
  }
  return result;
}

// Eigene Print-Klasse die Serial UND Ringpuffer beschreibt
class DualPrint : public Print {
  public:
    size_t write(uint8_t c) override {
      char buf[2] = {(char)c, 0};
      logAppend(buf);
      return Serial.write(c);
    }
    size_t write(const uint8_t *buffer, size_t size) override {
      for (size_t i = 0; i < size; i++) {
        char buf[2] = {(char)buffer[i], 0};
        logAppend(buf);
      }
      return Serial.write(buffer, size);
    }
};

DualPrint dualOut;

// Globaler Zeitstempel-Cache - wird nur einmal pro Sekunde aktualisiert
char tsCache[23] = "";
int tsLastSecond = -1;

void updateTimestampCache() {
  int s = second();
  if (s != tsLastSecond && year() > 2000) {
    tsLastSecond = s;
    int d = day(); int mo = month(); int y = year();
    int h = hour(); int mi = minute();
    tsCache[0] = '[';
    tsCache[1] = '0' + d / 10; tsCache[2] = '0' + d % 10; tsCache[3] = '.';
    tsCache[4] = '0' + mo / 10; tsCache[5] = '0' + mo % 10; tsCache[6] = '.';
    tsCache[7] = '0' + y / 1000; tsCache[8] = '0' + (y / 100) % 10;
    tsCache[9] = '0' + (y / 10) % 10; tsCache[10] = '0' + y % 10;
    tsCache[11] = ' ';
    tsCache[12] = '0' + h / 10; tsCache[13] = '0' + h % 10; tsCache[14] = ':';
    tsCache[15] = '0' + mi / 10; tsCache[16] = '0' + mi % 10; tsCache[17] = ':';
    tsCache[18] = '0' + s / 10; tsCache[19] = '0' + s % 10;
    tsCache[20] = ']'; tsCache[21] = ' '; tsCache[22] = 0;
  }
}

// Zeitstempel ausgeben - liest nur aus dem Cache, keine TimeLib-Aufrufe
void logTS() {
  if (tsCache[0] != 0) {
    Serial.print(tsCache);
    logAppend(tsCache);
  }
}

// Speicher-Info ausgeben
void logMemory() {
  logTS(); dualOut.print("[SYS] Freier Heap: ");
  dualOut.print(ESP.getFreeHeap());
  dualOut.print(" Bytes | Flash: ");
  dualOut.print(ESP.getSketchSize() / 1024);
  dualOut.print("KB / ");
  dualOut.print(ESP.getFreeSketchSpace() / 1024);
  dualOut.print("KB frei | Heap-Fragmentierung: ");
  dualOut.print(ESP.getHeapFragmentation());
  dualOut.println("%");
}

// Typedef fuer NeoPixelBus 2.8.4 - UART1 Methode (WiFi-kompatibel, nutzt GPIO2/D4)
typedef NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> NeoPixelBusType;

// Forward Declarations
void readDSTtime();
String StringIPaddress(IPAddress myaddr);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);
void interpretTimeZone(int timezonename);
void ChangeNeoPixels(uint16_t count, uint8_t pin);
void ssdpResponder();
void webHandleAlarm();
void webHandleReflection();
void webHandleDawn();
void webHandleMoon();
void gameface();

#define clockPin 4                //GPIO pin that the LED strip is on
const char* firmware_version = "2.2.0.37";
int pixelCount = 120;            //number of pixels in RGB clock


#define night 0                   // for switching between various clock modes
#define alarm 1
#define normal 2
#define dawnmode 3
#define game 4
int clockmode = normal;

//for switching various night clock modes
#define black 0
#define dots 1
#define dim 2
#define moonphase 3
#define disabled 4
#define moonmode 1
bool dawnbreak;
int hemisphere = 0; // 0=Nord, 1=Sued
int sleeptype = dots;

// Definiere deine NTP-Variablen (oft global)

#define gamestartpoints 20 //number of points a player starts with, this will determine how long a game lasts
//#define SECS_PER_HOUR 3600        //number of seconds in an hour

byte mac[6]; // MAC address
String macString;
String ipString;
String netmaskString;
String gatewayString;
String clockname = "mikotec-led-uhr";

IPAddress dns(8, 8, 8, 8);  //Google dns
const char* ssid = "MikoTec LED Uhr"; //The ssid when in AP mode
MDNSResponder mdns;
WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer server(80);
//ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
const byte DNS_PORT = 53;
//WiFiUDP UDP;
unsigned int localPort = 2390;      // local port to listen on for magic locator packets
char packetBuffer[255]; //buffer to hold incoming packet
char  ReplyBuffer[] = "I'm a light clock!";       // a string to send back

NeoPixelBusType* clockleds = NULL; // NeoPixelBus(pixelCount, clockPin);  //Clock Led on Pin 4
time_t getNTPtime(void);
//NTP NTPclient;
Ticker NTPsyncclock;
WiFiClient DSTclient;
Ticker alarmtick;
int alarmprogress = 0;
Ticker pulseBrightnessTicker;
Ticker gamestartticker;
int pulseBrightnessCounter =0;
Ticker dawntick;//a ticker to establish how far through dawn we are
int dawnprogress = 0;
const char* DSTTimeServer = "api.timezonedb.com";

// Auto-Update Konfiguration
const char* update_version_url = "http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/version.json";
const char* update_bin_base_url = "http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/";
unsigned long lastUpdateCheck = 0;
const unsigned long updateCheckInterval = 14400000; // 4 Stunden in Millisekunden (6x taeglich)

bool DSTchecked = 0;

// Sonnenzeiten API Cache (Open-Meteo)
int apiSunriseMinutes = -1;
int apiSunsetMinutes  = -1;
int apiCacheDay       = -1;



const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec
int webMode; //decides if we are in setup, normal or local only mode
const int debug = 0; //Set to one to get more log to serial
bool updateTime = true;
unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
int testrun;

//to be read from EEPROM Config
String esid = "";
String epass = "";


float latitude;
float longitude;

RgbColor hourcolor; // starting colour of hour
RgbColor minutecolor; //starting colour of minute
RgbColor alarmcolor; //the color the alarm will be
int brightness = 50; // a variable to dim the over-all brightness of the clock
int nightBrightness = 10; // Helligkeit im Schlafmodus (0-100)
int maxBrightness = 100;
uint8_t blendpoint = 40; //level of default blending
int randommode; //face changes colour every hour
int hourmarks = 1; //where marks should be made (midday/quadrants/12/brianmode)
int sleep = 22; //when the clock should go to night mode
int sleepmin = 0; //when the clock should go to night mode
int wake = 7; //when clock should wake again
int wakemin = 0; //when clock should wake again
int nightmode = 0;
int autoSleep = 0; // 0=manuell, 1=automatisch (Sonnenauf-/untergang)
unsigned long lastInteraction;

// Sonnenauf-/untergang Berechnung
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

float timezone = 10; //Australian Eastern Standard Time
int timezonevalue;
int DSTtime; //add one if we're in DST
bool showseconds; //should the seconds hand tick around
bool showSunPoint; //should the sun position be shown as a golden LED point
bool DSTauto; //should the clock automatically update for DST
int alarmmode = 0;
int gamearray[6];
RgbColor playercolors[6];
int playersremaining = 0;
int playercount = 0;
int nextplayer =0;
int gamebrightness = 100;
int speed = 0;
bool gamestarted = 0;
int loopcounter;

//new_moon = makeTime(0, 0, 0, 7, 0, 1970);

int accelerator = 5;
int prevsecond;
int hourofdeath; //saves the time incase of an unplanned reset
int minuteofdeath; //saves the time incase of an unplanned reset


// Vergleicht zwei Versionsnummern (z.B. "0.1" < "0.2")
// Gibt true zurueck wenn remoteVersion neuer ist als die lokale
bool isNewerVersion(const String& remoteVersion) {
  int lv[4] = {0, 0, 0, 0};
  int rv[4] = {0, 0, 0, 0};
  
  sscanf(firmware_version, "%d.%d.%d.%d", &lv[0], &lv[1], &lv[2], &lv[3]);
  sscanf(remoteVersion.c_str(), "%d.%d.%d.%d", &rv[0], &rv[1], &rv[2], &rv[3]);
  
  for (int i = 0; i < 4; i++) {
    if (rv[i] > lv[i]) return true;
    if (rv[i] < lv[i]) return false;
  }
  return false;
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
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, update_version_url);
  http.setTimeout(15000);
  
  logTS(); dualOut.println("[OTA] Sende HTTP-Anfrage...");
  int httpCode = http.GET();
  
  logTS(); dualOut.print("[OTA] HTTP-Antwort: ");
  dualOut.println(httpCode);
  
  if (httpCode != 200) {
    logTS(); dualOut.print("[OTA] Update-Check fehlgeschlagen, HTTP-Code: ");
    dualOut.println(httpCode);
    http.end();
    dualOut.println("==============================");
    return;
  }
  
  logTS(); dualOut.println("[OTA] version.json erfolgreich geladen.");
  String payload = http.getString();
  http.end();
  logTS(); dualOut.print("[OTA] Inhalt: ");
  dualOut.println(payload);
  
  // Version aus JSON parsen
  int vStart = payload.indexOf("\"version\"");
  if (vStart == -1) {
    logTS(); dualOut.println("[OTA] FEHLER: version.json ungueltig - kein version-Feld gefunden.");
    dualOut.println("==============================");
    return;
  }
  vStart = payload.indexOf("\"", vStart + 9) + 1;
  int vEnd = payload.indexOf("\"", vStart);
  String remoteVersion = payload.substring(vStart, vEnd);
  
  // Dateiname aus JSON parsen
  int fStart = payload.indexOf("\"file\"");
  if (fStart == -1) {
    logTS(); dualOut.println("[OTA] FEHLER: version.json ungueltig - kein file-Feld gefunden.");
    dualOut.println("==============================");
    return;
  }
  fStart = payload.indexOf("\"", fStart + 6) + 1;
  int fEnd = payload.indexOf("\"", fStart);
  String remoteFile = payload.substring(fStart, fEnd);
  
  logTS(); dualOut.print("[OTA] Installierte Version: ");
  dualOut.println(firmware_version);
  logTS(); dualOut.print("[OTA] Verfuegbare Version:  ");
  dualOut.println(remoteVersion);
  logTS(); dualOut.print("[OTA] Dateiname:            ");
  dualOut.println(remoteFile);
  
  if (!isNewerVersion(remoteVersion)) {
    logTS(); dualOut.println("[OTA] Firmware ist aktuell. Kein Update noetig.");
    dualOut.println("==============================");
    return;
  }
  
  String binUrl = String(update_bin_base_url) + remoteFile;
  logTS(); dualOut.println("[OTA] *** Neue Version gefunden! ***");
  logTS(); dualOut.print("[OTA] Download URL: ");
  dualOut.println(binUrl);
  logTS(); dualOut.print("[OTA] Freier Heap vor Update: ");
  dualOut.print(ESP.getFreeHeap());
  dualOut.println(" Bytes");
  logTS(); dualOut.println("[OTA] Starte Download und Flash...");
  
  WiFiClient updateClient;
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = ESPhttpUpdate.update(updateClient, binUrl);
  
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      logTS(); dualOut.print("[OTA] FEHLER: Update fehlgeschlagen (");
      dualOut.print(ESPhttpUpdate.getLastError());
      dualOut.print("): ");
      dualOut.println(ESPhttpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      logTS(); dualOut.println("[OTA] Server meldet: Kein Update verfuegbar.");
      break;
    case HTTP_UPDATE_OK:
      logTS(); dualOut.println("[OTA] Update erfolgreich! Neustart...");
      break;
  }
  dualOut.println("==============================");
}

//-----------------------------------standard arduino setup and loop-----------------------------------------------------------------------
void setup() {
  // 1. Serial zuerst
  Serial.begin(115200);
  delay(500);
  memset(logBuffer, 0, LOG_BUFFER_SIZE);
  logTS(); dualOut.println("");
  logTS(); dualOut.println("Light Clock wird gestartet...");
  logTS(); dualOut.print("Firmware Version: ");
  dualOut.println(firmware_version);

  // 2. Spielerfarben
  playercolors[0] = RgbColor(255, 0, 0);
  playercolors[1] = RgbColor(0, 255, 0);
  playercolors[2] = RgbColor(0, 0, 255);
  playercolors[3] = RgbColor(255, 255, 0);
  playercolors[4] = RgbColor(255, 0, 255);
  playercolors[5] = RgbColor(0, 255, 255);

  // 3. EEPROM laden
  EEPROM.begin(512);
  if (EEPROM.read(500) != 196) {
    writeInitalConfig();
  }
  loadConfig();
  delay(10);

  // 4. LEDs initialisieren (nach EEPROM damit pixelCount bekannt)
  if (pixelCount < 1 || pixelCount > 500) pixelCount = 120;
  logTS(); dualOut.println("Initialisiere LEDs...");
  if (clockleds) delete clockleds;
  clockleds = new NeoPixelBusType(pixelCount); // UART1 nutzt immer GPIO2 (D4)
  if (clockleds) {
    clockleds->Begin();
    clockleds->ClearTo(RgbColor(0, 0, 0));
    clockleds->Show();
    logTS(); dualOut.println("LEDs bereit.");
  }
  logMemory();

  // 5. Anzeige starten (nightCheck erst nach NTP, sonst falsches Datum)
  updateface();
  if(clockleds) clockleds->Show();
  initWiFi();
  lastInteraction = millis();
  //adjustTime(36600);
  delay(1000);
  // Setze den Zeit-Offset (Sekunden) erst hier im Setup
  NTPclient.setTimeOffset((timezone + DSTtime) * 3600);
  NTPclient.begin();
  if (DSTauto == 1) {
    readDSTtime();
  }
  //initialise the NTP clock sync function
  if (webMode == 1) {
    NTPclient.begin();
    NTPclient.forceUpdate();
    fetchSunriseSunset(latitude, longitude);
    time_t ntpTime = NTPclient.getEpochTime();
    logTS(); dualOut.print("NTP Erstabfrage: ");
    dualOut.println(ntpTime);
    if (ntpTime > 1000000) {
      setTime(ntpTime);
      updateTimestampCache();
      logTS(); dualOut.print("Zeit gesetzt auf: ");
      dualOut.print(hour());
      dualOut.print(":");
      dualOut.println(minute());
    }
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);

    macString = String(WiFi.macAddress());
    ipString = StringIPaddress(WiFi.localIP());
    netmaskString = StringIPaddress(WiFi.subnetMask());
    gatewayString = StringIPaddress(WiFi.gatewayIP());
  }
  //UDP.begin(localPort);
  prevsecond = second();// initalize prev second for main loop

  //update sleep/wake to current
  nightCheck();
  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  if (webMode == 1) {
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }

  // Update-Check wird nicht im Setup gemacht sondern verzögert im Loop
  // (mehr freier Heap nach dem Setup)
  if (webMode == 1) {
    lastUpdateCheck = millis() - updateCheckInterval + 30000; // Erster Check nach 30 Sekunden
    logTS(); dualOut.println("[OTA] Erster Update-Check in 30 Sekunden...");
  }

}

void loop() {
  if (webMode == 0) {
    //initiate web-capture mode
    dnsServer.processNextRequest();
  }
  if (webMode == 0 && millis() - lastInteraction > 300000) {//if the clock is looking for wifi network and hasn't found one after 5 minutes, and no one has tried to set it up, then reboot the clock
    lastInteraction = millis();

    ESP.reset();
  }
  webSocket.loop();
  server.handleClient();
  NTPclient.update();

  // Einmal taeglich auf Updates pruefen
  if (webMode == 1 && millis() - lastUpdateCheck > updateCheckInterval) {
    logTS(); dualOut.println("[OTA] Taeglicher Update-Check wird ausgefuehrt...");
    checkForUpdate();
    lastUpdateCheck = millis();
  }

  delay(50);
  // Log-Datei alle 5 Sekunden flushen
  static unsigned long lastLogFlush = 0;
  if (logFSready && logFile && millis() - lastLogFlush > 5000) {
    logFile.flush();
    lastLogFlush = millis();
  }
  if (second() != prevsecond) {
    updateTimestampCache();
    if (webMode != 0 && second() == 0 && minute() % 10 == 0) { //only record "time of death" if we're in normal running mode.
      EEPROM.begin(512);
      delay(10);
      EEPROM.write(193, hour());
      EEPROM.write(194, minute());
      EEPROM.write(191, brightness);
      EEPROM.commit();
      delay(50); // this section of code will save the "time of death" to the clock so if it unexpectedly resets it should be seemless to the user.
      saveFace(0);
    }
    if (second() == 0) {
      // Bei autoSleep: nightCheck alle 6 Stunden ausfuehren (0, 6, 12, 18 Uhr)
      // um Sonnenuntergang/Sonnenaufgang Werte zu aktualisieren
      if (autoSleep == 1 && minute() == 0 && hour() % 6 == 0) {
        if (clockmode != dawnmode) {
          nightCheck();
        }
      }

      if (hour() == sleep && minute() == sleepmin) {
        clockmode = night;
        logTS(); dualOut.println("Schlafmodus aktiviert");
      }
      if (hour() == wake && minute() == wakemin) {
        clockmode = normal;
        logTS(); dualOut.println("Aufwachmodus aktiviert");
      }
      if ((hour() + 25) % 24 == wake && minute() == wakemin) {
        if (dawnbreak) {
          dawnprogress = 0;
          clockmode = dawnmode;
          dawntick.attach(14, dawnadvance);
          logTS(); dualOut.println("Sonnenaufgang-Simulation gestartet");
        }
      }
    }
    prevsecond = second();
    bool skipDraw = (clockmode == night && sleeptype == moonphase && second() != 0);
    if (!skipDraw) {
      updateface();
      if (clockmode == night && sleeptype == moonphase) delay(1);
      if(clockleds) clockleds->Show();
    }
  }
  loopcounter++;


}



//--------------------UDP responder functions----------------------------------------------------

//void checkUDP(){
//  //dualOut.println("checking UDP");
//  // if there's data available, read a packet
//  int packetSize = UDP.parsePacket();
//  if (packetSize) {
//    dualOut.print("Received packet of size ");
//    dualOut.println(packetSize);
//    dualOut.print("From ");
//    IPAddress remoteIp = UDP.remoteIP();
//    dualOut.print(remoteIp);
//    dualOut.print(", port ");
//    dualOut.println(UDP.remotePort());
//
//    // read the packet into packetBufffer
//    int len = UDP.read(packetBuffer, 255);
//    if (len > 0) {
//      packetBuffer[len] = 0;
//    }
//    dualOut.println("Contents:");
//    dualOut.println(packetBuffer);
//
//    // send a reply, to the IP address and port that sent us the packet we received
//    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
//    UDP.write(ReplyBuffer);
//    UDP.endPacket();
//  }
//}


//--------------------EEPROM initialisations-----------------------------------------------------
void loadConfig() {
  logTS(); dualOut.println("reading settings from EEPROM");
  //Tries to read ssid and password from EEPROM
  EEPROM.begin(512);
  delay(10);

  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  logTS(); dualOut.print("SSID: ");
  dualOut.println(esid);


  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
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
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String s;

  String toSend = FPSTR(webconfig_html);
  //toSend.replace("$css", css_file);
  toSend.replace("$ssids", st);

  server.send(200, "text/html", toSend);
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
  String toSend = FPSTR(timezonesetup_html);
  //toSend.replace("$css", css_file);
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$latitude", String(latitude));
  toSend.replace("$longitude", String(longitude));

  server.send(200, "text/html", toSend);

  logTS(); dualOut.println("clearing old pass.");
  clearpass();


  String qpass;
  qpass = server.arg("pass");
  cleanASCII(qpass);
  dualOut.println(qpass);
  logTS(); dualOut.println("");

  //int addr=0;
  EEPROM.begin(512);
  delay(10);


  logTS(); dualOut.println("writing eeprom pass.");
  //addr += EEPROM.put(addr, qpass);
  for (int i = 0; i < qpass.length(); ++i)
  {
    EEPROM.write(32 + i, qpass[i]);
    dualOut.print(qpass[i]);
  }
  logTS(); dualOut.println("");
  EEPROM.write(186, 1);

  EEPROM.commit();
  delay(1000);
  EEPROM.end();

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
    readDSTtime();
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

int hexcolorToInt(char upper, char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal > 64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal > 64 ? lVal - 55 : lVal - 48;
  //dualOut.println(uVal+lVal);
  return uVal + lVal;
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


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
//------------------------------------------------animating functions-----------------------------------------------------------

void updateface() {
  //dualOut.println("Updating Face");
  int hour_pos;
  int min_pos;

  switch(speed) {
    case 0:
      hour_pos = ((hour() % 12) * pixelCount / 12 + minute() * pixelCount / 720);
      min_pos = (minute() * pixelCount / 60 + second() * pixelCount / 3600);
    break;

    case 1:
      hour_pos = ((minute() % 12) * pixelCount / 12 + second() * pixelCount / 720);
      min_pos = (second() * pixelCount / 60);
    break;

    case 2:
      hour_pos = ((10 % 12) * pixelCount / 12 + 10 * pixelCount / 720);
      min_pos = (10 * pixelCount / 60 + 0 * pixelCount / 3600);;
    break;
  }


  //dualOut.println("Main Switch");
  switch (clockmode) {



    case night:

      switch (sleeptype) {
        case black:
          for (int i = 0; i < pixelCount; i++) {
            clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
          }

          break;
        case dots:
          for (int i = 0; i < pixelCount; i++) {
            clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
          }
          clockleds->SetPixelColor(hour_pos, RgbColor::LinearBlend(RgbColor(0,0,0), hourcolor, (float)nightBrightness/255.0f));
          clockleds->SetPixelColor(min_pos, RgbColor::LinearBlend(RgbColor(0,0,0), minutecolor, (float)nightBrightness/255.0f));

          break;

        case dim:
          face(hour_pos, min_pos, _max(1, nightBrightness / 25));
          break;

        case moonphase:
          moon();
          clockleds->SetPixelColor((hour_pos + 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
          clockleds->SetPixelColor((hour_pos - 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
          clockleds->SetPixelColor((min_pos + 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
          clockleds->SetPixelColor((min_pos - 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
          clockleds->SetPixelColor(hour_pos, RgbColor::LinearBlend(RgbColor(0,0,0), hourcolor, (float)nightBrightness/255.0f));
          clockleds->SetPixelColor(min_pos, RgbColor::LinearBlend(RgbColor(0,0,0), minutecolor, (float)nightBrightness/255.0f));
          break;

        case disabled:
          face(hour_pos, min_pos);
          switch (hourmarks) {
            case 0:
              break;
            case 1:
              showMidday();
              break;
            case 2:
              showQuadrants();
              break;
            case 3:
              showHourMarks();
              break;
            case 4:
              darkenToMidday(hour_pos, min_pos);
          }
          //only show seconds in "day mode"
          if (showseconds) {

            invertLED(second()*pixelCount / 60);
          }
      }



      break;


    case alarm:
      alarmface();
      break;
    case game:
      gameface();
      break;

    case normal:
      face(hour_pos, min_pos);
      switch (hourmarks) {
        case 0:
          break;
        case 1:
          showMidday();
          break;
        case 2:
          showQuadrants();
          break;
        case 3:
          showHourMarks();
          break;
        case 4:
          darkenToMidday(hour_pos, min_pos);
      }
      //only show seconds in "day mode"
      if (showseconds) {

        invertLED(second()*pixelCount / 60);
      }
      if (showSunPoint) {
        int m = month();
        int d = day();
        int doy = d;
        int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int y = year();
        if (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) daysInMonth[2] = 29;
        for (int i = 1; i < m; i++) doy += daysInMonth[i];
        float tz = timezone + DSTtime;
        int sunriseMinutes, sunsetMinutes;
        getSunTimes(doy, latitude, longitude, tz, sunriseMinutes, sunsetMinutes);
        int nowMinutes = hour() * 60 + minute();
        if (nowMinutes >= sunriseMinutes && nowMinutes <= sunsetMinutes && sunsetMinutes > sunriseMinutes) {
          float sunProgress = (float)(nowMinutes - sunriseMinutes) / (float)(sunsetMinutes - sunriseMinutes);
          // Aufgang bei 3 Uhr (LED pixelCount/4), gegen Uhrzeigersinn über 12 nach 9 Uhr
          int sun_pos = ((pixelCount / 4) - (int)(sunProgress * (pixelCount / 2)) + pixelCount) % pixelCount;
          clockleds->SetPixelColor(sun_pos, RgbColor::LinearBlend(RgbColor(0,0,0), RgbColor(255, 180, 0), (float)brightness/255.0f));
        }
      }
      break;

    case dawnmode:
      dawn(dawnprogress);
      clockleds->SetPixelColor((hour_pos + 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
      clockleds->SetPixelColor((hour_pos - 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
      clockleds->SetPixelColor((min_pos + 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
      clockleds->SetPixelColor((min_pos - 1 + pixelCount) % pixelCount, RgbColor(0, 0, 0));
      clockleds->SetPixelColor(hour_pos, RgbColor::LinearBlend(RgbColor(0,0,0), hourcolor, (float)(std::min)(30, brightness)/255.0f));
      clockleds->SetPixelColor(min_pos, RgbColor::LinearBlend(RgbColor(0,0,0), minutecolor, (float)(std::min)(30, brightness)/255.0f));


  }


  //dualOut.println("Show LEDS");


}

void face(uint16_t hour_pos, uint16_t min_pos) {

  face(hour_pos, min_pos, brightness);
}

void face(uint16_t hour_pos, uint16_t min_pos, int bright) {
  HslColor c1, c1blend, c2, c2blend;
  int gap;
  int firsthand = min(hour_pos, min_pos);
  int secondhand = max(hour_pos, min_pos);
  if (hour_pos > min_pos) { c2 = HslColor(hourcolor); c1 = HslColor(minutecolor); }
  else { c1 = HslColor(hourcolor); c2 = HslColor(minutecolor); }
  c2blend = HslColor::LinearBlend<NeoHueBlendShortestDistance>(c2, c1, (float)blendpoint / 255);
  c1blend = HslColor::LinearBlend<NeoHueBlendShortestDistance>(c1, c2, (float)blendpoint / 255);
  gap = secondhand - firsthand;
  if (gap == 0) gap = 1;
  for (uint16_t i = firsthand; i < secondhand; i++) {
    clockleds->SetPixelColor(i, RgbColor::LinearBlend(RgbColor(0,0,0), RgbColor(HslColor::LinearBlend<NeoHueBlendShortestDistance>(c2blend, c2, ((float)i - (float)firsthand) / (float)gap)), (float)bright/255.0f));
  }
  gap = pixelCount - gap;
  if (gap == 0) gap = 1;
  for (uint16_t i = secondhand; i < pixelCount + firsthand; i++) {
    clockleds->SetPixelColor(i % pixelCount, RgbColor::LinearBlend(RgbColor(0,0,0), RgbColor(HslColor::LinearBlend<NeoHueBlendShortestDistance>(c1blend, c1, ((float)i - (float)secondhand) / (float)gap)), (float)bright/255.0f));
  }
  clockleds->SetPixelColor(hour_pos, RgbColor::LinearBlend(RgbColor(0,0,0), hourcolor, (float)bright/255.0f));
  clockleds->SetPixelColor(min_pos, RgbColor::LinearBlend(RgbColor(0,0,0), minutecolor, (float)bright/255.0f));
}
void nightface(uint16_t hour_pos, uint16_t min_pos) {
  for (int i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
  }
  clockleds->SetPixelColor(hour_pos, RgbColor::LinearBlend(RgbColor(0,0,0), hourcolor, (float)(std::min)(30, brightness)/255.0f));
  clockleds->SetPixelColor(min_pos, RgbColor::LinearBlend(RgbColor(0,0,0), minutecolor, (float)(std::min)(30, brightness)/255.0f));

}

void alarmface() {
  RgbColor redblack;
  if (alarmprogress == pixelCount) {//flash the face when alarm is finished
    (second() % 2) ? redblack = alarmcolor : redblack = RgbColor(0, 0, 0);
    for (int i = 0; i < pixelCount; i++) {
      clockleds->SetPixelColor(i, redblack);
    }
  }
  else {
    for (int i = 0; i < alarmprogress; i++) {
      clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
    }
    for (int i = alarmprogress; i < pixelCount; i++) {
      clockleds->SetPixelColor(i, alarmcolor);
    }
  }


}


void alarmadvance() {
  //dualOut.println("advancing alarm");

  if (alarmprogress != pixelCount) {
    alarmprogress++;
    updateface();

  } else {
    alarmtick.detach();
  }
  //    alarmtick.attach(0.3, flashface);
  //    alarmprogress = 0;
  //
  //  }
  //
}

//void flashface() {
//  alarmmode = 2;
//  if (alarmprogress == 10) {
//    alarmtick.detach();
//    alarmprogress = 0;
//    clockmode = normal;
//  } else {
//    if ((alarmprogress % 2) == 0) {
//      for (int i = 0; i < pixelCount; i++) {
//        clockleds->SetPixelColor(i, 255, 0, 0);
//      }
//    } else {
//      for (int i = 0; i < pixelCount; i++) {
//        clockleds->SetPixelColor(i, 0, 0, 0);
//      }
//    }
//  }
//
//  alarmprogress++;
//  updateface();
//}

void invertLED(int i) {
  //This function will set the LED to in inverse of the two LEDs next to it showing as white on the main face
  RgbColor averagecolor;
  averagecolor = RgbColor::LinearBlend(clockleds->GetPixelColor((i - 1) % pixelCount), clockleds->GetPixelColor((i + 1) % pixelCount), 0.5f);
  averagecolor = RgbColor(255 - averagecolor.R, 255 - averagecolor.G, 255 - averagecolor.B);
  clockleds->SetPixelColor(i, RgbColor::LinearBlend(RgbColor(0,0,0), averagecolor, (float)brightness/255.0f));
}

void showHourMarks() {
  //shows white at the four quadrants and darkens each hour mark to help the user tell the time
  //  RgbColor c;
  //  for (int i = 0; i < 12; i++) {
  //    c = clockleds->GetPixelColor(i);
  //    c.Darken(255);
  //    clockleds->SetPixelColor(i * pixelCount / 12, c,brightness);
  //  }

  for (int i = 0; i < 12; i++) {
    invertLED(i * pixelCount / 12);
  }
}

void showQuadrants() {
  //shows white at each of the four quadrants to orient the user
  for (int i = 0; i < 4; i++) {
    invertLED(i * pixelCount / 4);
  }
}

void showMidday() {
  //shows a bright light at midday to orient the user
  invertLED(0);
}

void darkenToMidday(uint16_t hour_pos, uint16_t min_pos) {
  //darkens the pixels between the second hand and midday because Brian suggested it.
  int secondhand = _max(hour_pos, min_pos);
  RgbColor c;
  for (uint16_t i = secondhand; i < pixelCount; i++) {
    c = clockleds->GetPixelColor(i);
    c.Darken(240);
    clockleds->SetPixelColor(i, c);
  }
}

//void nightModeAnimation() {
//  //darkens the pixels animation to switch to nightmode.
////  int firsthand = (std::min)(hour_pos, min_pos);
////  int secondhand = (max)(hour_pos, min_pos);
////  int firsthandlen = (120+firsthand-secondhand)%120;
////  int secondhandlen = 120-firsthandlen;
//
//
//
//  RgbColor c;
//
//  for (uint16_t i = 0; i < 240; i++) {
//    for (uint16_t j = 0; j < (std::min)(i, (uint16_t)120); i++) {
//    c = clockleds->GetPixelColor(i);
//    c.Darken(20);
//    clockleds->SetPixelColor(i, c);
//
//    }
//
//    delay(10);
//  }
//}

void logo() {
  //this lights up the clock as the C logo
  //yellow section
  for (int i = 14 / (360 / pixelCount); i < 48 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, RgbColor(100, 100, 0));
  }

  //blank section
  for (int i = 48 / (360 / pixelCount); i < 140 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
  }

  //blue section
  for (int i = 140 / (360 / pixelCount); i < 296 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, RgbColor(0, 60, 120));
  }

  //green section
  for (int i = 296 / (360 / pixelCount); i < (360 + 14) / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i % pixelCount, RgbColor(30, 120, 0));
  }


}

void pulseBrightness() {
  pulseBrightnessCounter++;
  if(pulseBrightnessCounter == 10){
    pulseBrightnessCounter = 0;
    brightness = brightness+18;
  } else {
    brightness = brightness -2;
  }
  updateface();

}

void sparkles() {
  updateface();
  int darkled[pixelCount];
  memset(darkled, 0 , sizeof(darkled));//initialize all leds to off


    for (int i = 0; i< pixelCount * 0.75; i++){
      int ledToTurnOn = random(pixelCount-i); // choose a random pixel to turn on from the remaining off pixels
      int k = 0;
      while(k <= ledToTurnOn){
        ledToTurnOn += darkled[k]; // skip over the already on LEDs
        k++;
      }
      darkled[ledToTurnOn] = 1;
    }


      for (int j = 0; j < pixelCount; j++) {
        if(darkled[j]==0){
          clockleds->SetPixelColor(j, RgbColor(0, 0, 0)); //blacken the LED if it's dark in the array
        }
      }


}

void dawnadvance() {
  if (dawnprogress == 255) {
    clockmode = normal;
    dawntick.detach();
    dawnprogress = 0;
  }
  else {
    dawnprogress++;
  }
}
void dawn(int i) {//this sub will present a dawning sun with the time highlighted in dots. I should vary from 0 to 255
  RgbColor  c1 = RgbColor(255, 142, 0);
  int bright;
  int green;
  int blue = 0;

  if (i < 142) {
    bright = i * 64 / 142;
  } else if (i >= 142 && i < 204) {
    bright = 64 + (i - 142) * 128 / 62;
  } else {
    bright = 192 + (i - 204) * 64 / 51;
  }



  green = _max(142, i);

  if (i > 204) {
    blue = (5 * i - 1020);
  } else {
    blue = 0;
  }

  for (int j = 0; j < pixelCount; j++) {
    if (j < (i * pixelCount / 280) || j > (pixelCount - (i * pixelCount / 280))) {
      clockleds->SetPixelColor(j, RgbColor::LinearBlend(RgbColor(0,0,0), RgbColor(255, green, blue), (float)bright/255.0f));
    }
    else {
      clockleds->SetPixelColor(j, RgbColor(0, 0, 0));
    }
  }

}
void dawntest() {
  RgbColor  c1 = RgbColor(255, 142, 0);
  int bright;
  int green;
  int blue = 0;

  for (int i = 0; i < 255; i++) {

    if (i < 142) {
      bright = i * 64 / 142;
    } else if (i >= 142 && i < 204) {
      bright = 64 + (i - 142) * 128 / 62;
    } else {
      bright = 192 + (i - 204) * 64 / 51;
    }



    green = _max(142, i);

    if (i > 204) {
      blue = (5 * i - 1020);
    } else {
      blue = 0;
    }

    for (int j = 0; j < pixelCount; j++) {
      if (j < (i * pixelCount / 280) || j > (pixelCount - (i * pixelCount / 280))) {
        clockleds->SetPixelColor(j, RgbColor::LinearBlend(RgbColor(0,0,0), RgbColor(255, green, blue), (float)bright/255.0f));
      }
      else {
        clockleds->SetPixelColor(j, RgbColor(0, 0, 0));
      }
    }

    clockleds->Show();
    delay(100);
  }
  for (int j = 0; j < pixelCount; j++) {
    clockleds->SetPixelColor(j, RgbColor(20, 20, 20));

  }

}

void moontest() {
  long lp = 2551443L;
  long ref_new_moon = 947182440L; // Referenz-Neumond 6. Jan 2000 18:14 UTC
  long diff = (now() - ref_new_moon) % lp; if (diff < 0) diff += lp; int phase = (int)(diff / 86400L);
  logTS(); dualOut.print("phase: ");
  dualOut.println(phase);

  for (phase = 0; phase < 30; phase++) {
    float illumination = (1.0 - cos(2.0 * PI * phase / 29.53)) / 2.0;
    int litLEDs = (int)(illumination * pixelCount);
    logTS(); dualOut.print("phase: "); dualOut.print(phase);
    logTS(); dualOut.print(" illumination: "); dualOut.print((int)(illumination*100));
    logTS(); dualOut.print("% litLEDs: "); dualOut.println(litLEDs);

    // Alle LEDs aus
    for (int i = 0; i < pixelCount; i++) {
      clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
    }
    // Beleuchtete LEDs setzen
    bool waxing = (phase < 15);
    int moonCenter = pixelCount / 2;
    int fadeZone = pixelCount / 20;
    if (fadeZone < 2) fadeZone = 2;
    for (int i = 0; i < litLEDs + fadeZone * 2; i++) {
      int ledIdx;
      if (hemisphere == 0) {
        ledIdx = waxing ?
          (moonCenter + litLEDs / 2 - i + pixelCount) % pixelCount :
          (moonCenter - litLEDs / 2 + i + pixelCount) % pixelCount;
      } else {
        ledIdx = waxing ?
          (moonCenter - litLEDs / 2 + i + pixelCount) % pixelCount :
          (moonCenter + litLEDs / 2 - i + pixelCount) % pixelCount;
      }
      int bv;
      if (i < fadeZone) { bv = 64 * i / fadeZone; }
      else if (i >= litLEDs + fadeZone) { bv = 64 * (litLEDs + fadeZone * 2 - i) / fadeZone; }
      else { bv = 64; }
      if (bv > 0) clockleds->SetPixelColor(ledIdx, RgbColor(bv, bv, bv));
    }
    clockleds->Show();
    delay(1000);
  }

}
void moon() {
  long lp = 2551443L; // Synodischer Monat in Sekunden (29.53058770576 Tage)
  long ref_new_moon = 947182440L; // Referenz-Neumond 6. Jan 2000 18:14 UTC (astronomisch verifiziert)

  // Phase berechnen: Tage seit letztem Neumond als Fliesskomma fuer genauere Beleuchtung
  long diff = (now() - ref_new_moon) % lp;
  if (diff < 0) diff += lp;
  float phase_days = (float)diff / 86400.0;
  int phase = (int)phase_days; // 0=Neumond, ~7=erstes Viertel, ~14=Vollmond, ~22=letztes Viertel

  // Beleuchtung mit Fliesskomma-Phase fuer glattere Uebergaenge
  float illumination = (1.0 - cos(2.0 * PI * phase_days / 29.53)) / 2.0;

  // Anzahl beleuchteter LEDs basierend auf Beleuchtung
  int litLEDs = (int)(illumination * pixelCount);

  // Position: Nordhalbkugel wächst rechts, Südhalbkugel wächst links
  // phase < 15 = zunehmend, phase >= 15 = abnehmend
  bool waxing = (phase < 15);

  // Startposition der beleuchteten Seite
  int moonCenter = pixelCount / 2; // Mitte oben (12-Uhr-Position)
  int startLED;

  if (hemisphere == 0) {
    // Nordhalbkugel: zunehmend = rechte Seite beleuchtet
    if (waxing) {
      startLED = moonCenter - litLEDs / 2;
    } else {
      startLED = moonCenter - litLEDs / 2;
    }
  } else {
    // Südhalbkugel: gespiegelt
    if (waxing) {
      startLED = moonCenter - litLEDs / 2;
    } else {
      startLED = moonCenter - litLEDs / 2;
    }
  }

  // Alle LEDs erstmal aus
  for (int i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, RgbColor(0, 0, 0));
  }

  // Beleuchtete LEDs setzen mit weichem Rand
  int fadeZone = pixelCount / 20; // sanfter Übergang am Rand
  if (fadeZone < 2) fadeZone = 2;

  for (int i = 0; i < litLEDs + fadeZone * 2; i++) {
    int ledIdx;
    if (hemisphere == 0) {
      ledIdx = waxing ?
        (moonCenter + litLEDs / 2 - i + pixelCount) % pixelCount :
        (moonCenter - litLEDs / 2 + i + pixelCount) % pixelCount;
    } else {
      ledIdx = waxing ?
        (moonCenter - litLEDs / 2 + i + pixelCount) % pixelCount :
        (moonCenter + litLEDs / 2 - i + pixelCount) % pixelCount;
    }

    int brightness_val;
    int moonMax = _max(1, 64 * nightBrightness / 100); // Mond-Helligkeit nach nightBrightness skalieren
    if (i < fadeZone) {
      brightness_val = moonMax * i / fadeZone; // einblenden
    } else if (i >= litLEDs + fadeZone) {
      brightness_val = moonMax * (litLEDs + fadeZone * 2 - i) / fadeZone; // ausblenden
    } else {
      brightness_val = moonMax; // voll beleuchtet
    }

    if (brightness_val > 0) {
      clockleds->SetPixelColor(ledIdx, RgbColor(brightness_val, brightness_val, brightness_val));
    }
  }

  // Debug nur einmal pro Stunde ausgeben
  static int lastMoonLogHour = -1;
  if (hour() != lastMoonLogHour) {
    lastMoonLogHour = hour();
    logTS(); dualOut.print("Mondphase Tag: "); dualOut.print(phase);
    dualOut.print(" Beleuchtung: "); dualOut.print((int)(illumination * 100));
    dualOut.print("% LEDs: "); dualOut.println(litLEDs);
    logMemory();
  }
}

void brighttest() {
  for (int i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, RgbColor(i, i, i));
  }

  delay(10000);
}

void lightup() {
  int darkled[pixelCount];
  memset(darkled, 0 , sizeof(darkled));//initialize all leds to off
  server.send(200, "text/html", "<form class=form-verticle action=/lightup method=GET> Skip check /p <input type=number name=skip>/p <input type=submit name=submit value='Save Settings'/>");
  if (server.hasArg("skip")) {

    String skipstring = server.arg("skip");  //get value input
    int skip = skipstring.toInt();
    randomSeed(skip); //seed just incase we find one we particularly like/don't like
    for (int i = 0; i < pixelCount; i++) {

      int ledToTurnOn = random(pixelCount-i); // choose a random pixel to turn on from the remaining off pixels
      int k = 0;
      while(k <= ledToTurnOn){
        ledToTurnOn += darkled[k]; // skip over the already on LEDs
        k++;
      }
      darkled[ledToTurnOn] = 1;

      face(10, 50);
      for (int j = 0; j < pixelCount; j++) {
        if(darkled[j]==0){
          clockleds->SetPixelColor(j, RgbColor(0, 0, 0)); //blacken the LED if it's dark in the array
        }
      }
      clockleds->Show();
      delay(_max((pow(pixelCount - i, 7) / pow(pixelCount, 7)) * 1000, 40));

    }
  }
  delay(5000);

}
//------------------------------EEPROM save/read functions-----------------------

void writeLatLong(int partition, float latlong) {
  int val = (int16_t)(latlong * 182);

  EEPROM.write(partition, (val & 0xff));
  EEPROM.write(partition + 1, ((val >> 8) & 0xff));

}

float readLatLong(int partition) {
  EEPROM.begin(512);
  delay(10);
  int16_t val = EEPROM.read(partition) | (EEPROM.read(partition + 1) << 8);

  return (float)val / 182;
}

void saveFace(uint8_t partition)
{
  if (partition >= 0 && partition <= 4) { // only 3 locations for saved faces. Don't accidentally overwrite other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color

    EEPROM.write(100 + partition * 15, hourcolor.R);
    EEPROM.write(101 + partition * 15, hourcolor.G);
    EEPROM.write(102 + partition * 15, hourcolor.B);


    //write the minute color
    EEPROM.write(103 + partition * 15, minutecolor.R);
    EEPROM.write(104 + partition * 15, minutecolor.G);
    EEPROM.write(105 + partition * 15, minutecolor.B);


    //write the blend point
    EEPROM.write(106 + partition * 15, blendpoint);

    EEPROM.commit();
    delay(500);
  }
}


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
    json += "{\"h\":\"" + String(sh) + "\",\"m\":\"" + String(sm) + "\"}";
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
  json += "\"timezone\":" + String(timezone);
  json += "}";
  logTS(); dualOut.println("[SETTINGS] " + json);
  server.send(200, "application/json", json);
}

void webHandleTimeSet() {
  if (server.hasArg("time")) {
    String timestring = server.arg("time");
    int timehr = timestring.substring(0, 2).toInt();
    int timemin = timestring.substring(3, 5).toInt();
    int timesec = 0;
    if (timestring.length() >= 8) {
      timesec = timestring.substring(6, 8).toInt();
    }
    logTS(); dualOut.print("Time Total: "); dualOut.println(timestring);
    logTS(); dualOut.print("Time Hour: "); dualOut.println(timehr);
    logTS(); dualOut.print("Time Minute: "); dualOut.println(timemin);
    logTS(); dualOut.print("Time Second: "); dualOut.println(timesec);
    setTime(timehr, timemin, timesec, 1, 1, 1);
  }
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
void connectToDSTServer() {
  String GETString;
  // attempt to connect, and wait a millisecond:

  logTS(); dualOut.println("Connecting to DST server");
  DSTclient.connect("api.timezonedb.com", 80);

  if (DSTclient.connect("api.timezonedb.com", 80)) {
    // make HTTP GET request to timezonedb.com:
    GETString += "GET /?lat=";
    GETString += latitude;
    GETString += "&lng=";
    GETString += longitude;
    GETString += "&key=AX6GA4Y3762L HTTP/1.1";

    DSTclient.println(GETString);
    dualOut.println(GETString);
    DSTclient.println("Host: api.timezonedb.com");
    logTS(); dualOut.println("Host: api.timezonedb.com");
    DSTclient.println("Connection: close\r\n");
    logTS(); dualOut.println("Connection: close\r\n");
    //DSTclient.print("Accept-Encoding: identity\r\n");
    //DSTclient.print("Host: api.geonames.org\r\n");
    //DSTclient.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");
    //DSTclient.print("Connection: close\r\n\r\n");

    int i = 0;
    while ((!DSTclient.available()) && (i < 1000)) {
      delay(10);
      i++;
    }
  }
}

void readDSTtime() {
  float oldtimezone = timezone;
  String currentLine = "";
  bool readingUTCOffset = false;
  String UTCOffset;
  connectToDSTServer();
  logTS(); dualOut.print("DST.connected: ");
  dualOut.println(DSTclient.connected());
  logTS(); dualOut.print("DST.available: ");
  dualOut.println(DSTclient.available());

  while (DSTclient.connected()) {
    if (DSTclient.available()) {

      // read incoming bytes:
      char inChar = DSTclient.read();
      // add incoming byte to end of line:
      currentLine += inChar;

      // if you're currently reading the bytes of a UTC offset,
      // add them to the UTC offset String:
      if (readingUTCOffset) {//the section below has flagged that we're getting the UTC offset from server here
        if (inChar != '<') {
          UTCOffset += inChar;
        }
        else {
          // if you got a "<" character,
          // you've reached the end of the UTC offset:
          readingUTCOffset = false;
          logTS(); dualOut.print("UTC Offset in seconds: ");
          dualOut.println(UTCOffset);
          //update the internal time-zone
          timezone = UTCOffset.toInt() / 3600;
          adjustTime((timezone - oldtimezone) * 3600);
          NTPclient.setTimeOffset((timezone + DSTtime) * 3600);
          //setTime(NTPclient.getEpochTime());

          // close the connection to the server:
          DSTclient.stop();
        }
      }

      // if you get a newline, clear the line:
      if (inChar == '\n') {

        dualOut.println(currentLine);
        currentLine = "";
      }
      // if the current line ends with <text>, it will
      // be followed by the tweet:
      if ( currentLine.endsWith("<gmtOffset>")) {
        // UTC offset is beginning. Clear the tweet string:

        dualOut.println(currentLine);
        readingUTCOffset = true;
        UTCOffset = "";
      }


    }
  }
}

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
