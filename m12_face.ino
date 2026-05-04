// m12_face.ino

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

  // Sicherstellen dass litLEDs + fadeZone*2 nicht pixelCount übersteigt
  int fadeZone = pixelCount / 20;
  if (fadeZone < 2) fadeZone = 2;
  if (litLEDs > pixelCount - fadeZone * 2) litLEDs = pixelCount - fadeZone * 2;

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
