// m02_timestamp.ino

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
