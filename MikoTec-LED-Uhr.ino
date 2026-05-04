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
#include <PubSubClient.h>

WiFiUDP ntpUDP;
// Hier nur ganz einfach ohne die Berechnungen:
NTPClient NTPclient(ntpUDP, "pool.ntp.org");

// ---- MQTT ----
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
bool mqttEnabled = false;
bool betaChannel = false; // EEPROM 367: Beta-Updates aktivieren
char mqttBroker[64] = "";
int mqttPort = 1883;
char mqttUser[32] = "";
char mqttPass[32] = "";
unsigned long lastMqttReconnect = 0;
unsigned long lastMqttPublish = 0;
bool mqttDiscoverySent = false;
int mqttPrevClockmode = -1;
int mqttPrevSleeptype = -1;
String mqttAvailableVersion = "";
String mqttAvailableFile = "";
// EEPROM Layout MQTT (ab 236):
// 236: mqttEnabled (1 byte)
// 237-238: mqttPort (2 bytes, high/low)
// 239-302: mqttBroker (64 bytes)
// 303-334: mqttUser (32 bytes)
// 335-366: mqttPass (32 bytes)

#ifndef _max
#define _max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef _min
#define _min(a,b) ((a)<(b)?(a):(b))
#endif

// Serial-Ringpuffer fuer Support-Seite
#define LOG_BUFFER_SIZE 4096
char logBuffer[LOG_BUFFER_SIZE];
int logWritePos = 0;
bool logWrapped = false;
bool logFSready = false;  // true sobald LittleFS bereit und Log-Datei offen
File logFile;             // globales Handle fuer /log.txt in LittleFS


// Eigene Print-Klasse die Serial UND Ringpuffer beschreibt
// MQTT Log Zeilenpuffer
char mqttLogLine[256];
int mqttLogLinePos = 0;


// Forward Declarations - alle Funktionen aus Modul-Dateien
void logAppend(const char* str);
String getLogContent();
void mqttLogPublish();
void updateTimestampCache();
void logTS();
void logMemory();
void fetchSunriseSunset(float lat, float lng);
void getSunTimes(int dayOfYear, float lat, float lng, float tz, int &sunriseMin, int &sunsetMin);
void calcSunriseSunset(int dayOfYear, float lat, float lng, float tz, int &srH, int &srM, int &ssH, int &ssM);
bool isNewerVersion(const String& remoteVersion);
void checkForUpdate();
String mqttBaseTopic();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttPublishUpdateState();
void mqttPublishState();
void mqttPublishDiscovery();
bool mqttReconnect();
void loadMqttConfig();
void saveMqttConfig();
void handleGetMqtt();
void handleSetMqtt();
void loadConfig();
void writeInitalConfig();
void initWiFi();
int testWifi(void);
void setupAP(void);
void launchWeb(int webtype);
void setUpServerHandle();
void speedup();
void webHandleSwitchWebMode();
void webHandleConfig();
void webHandlePassword();
void cleanASCII(String &input);
void webHandleTimeZoneSetup();
void webHandleConfigSave();
void handleNotFound();
void handleCSS();
void handlecolourjs();
void handlespectrumjs();
void handleclockjs();
void handlespectrumCSS();
void handleRoot();
void nightCheck();
void handleSettings();
void handleTimezone();
void webHandleClearRom();
void webHandleClearRomSure();
void getRGB(String hexRGB, RgbColor &rgb);
String rgbToText(RgbColor input);
String timeToText(int hours, int minutes);
void updateface();
void face(uint16_t hour_pos, uint16_t min_pos);
void face(uint16_t hour_pos, uint16_t min_pos, int bright);
void nightface(uint16_t hour_pos, uint16_t min_pos);
void alarmface();
void alarmadvance();
void invertLED(int i);
void showHourMarks();
void showQuadrants();
void showMidday();
void darkenToMidday(uint16_t hour_pos, uint16_t min_pos);
void logo();
void pulseBrightness();
void sparkles();
void dawnadvance();
void dawn(int i);
void dawntest();
void moontest();
void moon();
void brighttest();
void lightup();
void writeLatLong(int partition, float latlong);
float readLatLong(int partition);
void saveFace(uint8_t partition);
void loadFace(uint8_t partition);
void clearEEPROM();
void clearssid();
void clearpass();
void webHandleNightModeDemo();
void webHandleGame();
void handleHilfe();
void handleSupport();
void handleGetLog();
void handleGetSysInfo();
void handleReboot();
void handleGetTime();
void handleGetState();
void handleGetSettings();
void webHandleTimeSet();
void webHandleReflection();
void webHandleDawn();
void webHandleMoon();
void webHandleAlarm();
void ssdpResponder();
void interpretTimeZone(int timezonename);
void ChangeNeoPixels(uint16_t count, uint8_t pin);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);
String StringIPaddress(IPAddress myaddr);
void gamestart();
void gamecountdown();
void gamejoin(int num);
void gameplus(int playernum);
void animatewinner(int winner);
void gameface();


class DualPrint : public Print {
  public:
    size_t write(uint8_t c) override {
      char buf[2] = {(char)c, 0};
      logAppend(buf);
      // MQTT Log: Zeichen sammeln, bei Newline publishen
      if (c == '\n') {
        mqttLogPublish();
      } else if (mqttLogLinePos < 254) {
        mqttLogLine[mqttLogLinePos++] = (char)c;
      }
      return Serial.write(c);
    }
    size_t write(const uint8_t *buffer, size_t size) override {
      for (size_t i = 0; i < size; i++) {
        char buf[2] = {(char)buffer[i], 0};
        logAppend(buf);
        if (buffer[i] == '\n') {
          mqttLogPublish();
        } else if (mqttLogLinePos < 254) {
          mqttLogLine[mqttLogLinePos++] = (char)buffer[i];
        }
      }
      return Serial.write(buffer, size);
    }
};

DualPrint dualOut;

// Globaler Zeitstempel-Cache - wird nur einmal pro Sekunde aktualisiert
char tsCache[23] = "";
int tsLastSecond = -1;


// Typedef fuer NeoPixelBus 2.8.4 - UART1 Methode (WiFi-kompatibel, nutzt GPIO2/D4)
typedef NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> NeoPixelBusType;

#define clockPin 4                //GPIO pin that the LED strip is on
const char* firmware_version = "2.3.0.33";
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
Ticker alarmtick;
int alarmprogress = 0;
Ticker pulseBrightnessTicker;
Ticker gamestartticker;
int pulseBrightnessCounter =0;
Ticker dawntick;//a ticker to establish how far through dawn we are
int dawnprogress = 0;

// Auto-Update Konfiguration
const char* update_version_url = "http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/version.json";
const char* update_bin_base_url = "http://yzdlcru01ktmqlzy.myfritz.net:8080/updates/";
unsigned long lastUpdateCheck = 0;
int lastUpdateCheckHour = -1; // Letzte Stunde in der ein Check lief

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
  //initialise the NTP clock sync function
  if (webMode == 1) {
    NTPclient.begin();
    // NTP: 2 schnelle Versuche ohne blockierenden Delay
    time_t ntpTime = 0;
    for (int ntpTry = 0; ntpTry < 2; ntpTry++) {
      NTPclient.forceUpdate();
      ntpTime = NTPclient.getEpochTime();
      if (ntpTime > 1000000) break;
    }
    fetchSunriseSunset(latitude, longitude);
    if (ntpTime > 1000000) {
      setTime(ntpTime);
      updateTimestampCache();
      logTS(); dualOut.print("NTP Zeit gesetzt: ");
      dualOut.print(hour());
      dualOut.print(":");
      dualOut.println(minute());
    } else {
      logTS(); dualOut.println("NTP: Keine gueltige Zeit erhalten!");
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

  // Update-Check: erster Check 30 Sekunden nach Boot
  if (webMode == 1) {
    lastUpdateCheck = millis();
    logTS(); dualOut.println("[OTA] Erster Update-Check in 30 Sekunden...");
    logTS(); dualOut.println("[OTA] Planmaessige Checks: 0, 3, 6, 9, 12, 15, 18, 21 Uhr");
  }

  // MQTT initialisieren wenn aktiviert und WiFi verbunden
  if (mqttEnabled && webMode == 1 && strlen(mqttBroker) > 0) {
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1200);
    logTS(); dualOut.println("[MQTT] Client konfiguriert: " + String(mqttBroker) + ":" + String(mqttPort));
  }

}

// ---- MQTT Funktionen ----








// Handler fuer /getmqtt und /setmqtt Endpunkte

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

  // MQTT loop
  if (mqttEnabled && webMode == 1 && strlen(mqttBroker) > 0) {
    if (mqttClient.connected()) {
      mqttClient.loop();
      // State alle 30 Sekunden publishen
      if (millis() - lastMqttPublish > 30000) {
        mqttPublishState();
        lastMqttPublish = millis();
      }
    } else {
      // Reconnect alle 10 Sekunden versuchen
      if (millis() - lastMqttReconnect > 10000) {
        mqttReconnect();
        lastMqttReconnect = millis();
      }
    }
  }

  // Update-Check: erster Check 30 Sekunden nach Boot, dann zu festen Uhrzeiten
  if (webMode == 1) {
    // Einmaliger Boot-Check nach 30 Sekunden
    if (lastUpdateCheckHour == -1 && millis() - lastUpdateCheck > 30000) {
      lastUpdateCheckHour = -2; // markiert: Boot-Check erledigt
      logTS(); dualOut.println("[OTA] Boot Update-Check...");
      checkForUpdate();
    }
    // Planmaessige Checks: 0, 3, 6, 9, 12, 15, 18, 21 Uhr
    int h = hour();
    if (h % 3 == 0 && h != lastUpdateCheckHour && minute() == 0) {
      lastUpdateCheckHour = h;
      logTS(); dualOut.println("[OTA] Planmaessiger Update-Check (" + String(h) + ":00 Uhr)...");
      checkForUpdate();
    }
    // Reset wenn Stunde sich aendert (damit naechster 3h-Slot wieder triggert)
    if (h % 3 != 0 && lastUpdateCheckHour >= 0) {
      lastUpdateCheckHour = h;
    }
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

    //write the brightness
    EEPROM.write(107 + partition * 15, brightness);

    EEPROM.commit();
    delay(500);
  }
}


