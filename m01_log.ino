// m01_log.ino

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

void mqttLogPublish() {
  if (mqttLogLinePos == 0) return;
  mqttLogLine[mqttLogLinePos] = '\0';
  if (mqttEnabled && mqttClient.connected()) {
    String topic = mqttBaseTopic() + "/log";
    mqttClient.publish(topic.c_str(), mqttLogLine);
  }
  mqttLogLinePos = 0;
}
