#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "battery.h"

RTC_DATA_ATTR static uint32_t bootCount = 0;

void initLogger() {
  if (FORMAT_LITTLEFS) {
    LittleFS.format();
    LittleFS.begin();
    File marker = LittleFS.open("/.formatted", FILE_WRITE);
    if (marker) {
      marker.print("formatted");
      marker.close();
    }
    Serial.println("LittleFS formatted");
  } else {
    if (!LittleFS.begin(false)) {
      if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
      }
      File marker = LittleFS.open("/.formatted", FILE_WRITE);
      if (marker) {
        marker.print("formatted");
        marker.close();
      }
      Serial.println("LittleFS initialized");
    } else if (!LittleFS.exists("/.formatted")) {
      File marker = LittleFS.open("/.formatted", FILE_WRITE);
      if (marker) {
        marker.print("formatted");
        marker.close();
      }
    }
  }
  bootCount++;
  File bootLog = LittleFS.open("/boot.csv", FILE_APPEND);
  if (!bootLog) {
    Serial.println("WARN: Creating new boot.csv");
    bootLog = LittleFS.open("/boot.csv", FILE_WRITE);
    if (bootLog) {
      bootLog.println("timestamp,boot_count");
      bootLog.close();
      bootLog = LittleFS.open("/boot.csv", FILE_APPEND);
    }
  }
  if (bootLog) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char ts[20];
      strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &timeinfo);
      bootLog.printf("%s,%lu\n", ts, (unsigned long)bootCount);
    } else {
      bootLog.printf("--:--,%lu\n", (unsigned long)bootCount);
    }
    bootLog.close();
  } else {
    Serial.println("ERR: Cannot write boot.csv");
  }
}

void logBatteryData() {
  if (!LittleFS.begin(false)) {
    Serial.println("WARN: LittleFS mount failed, reformatting...");
    if (!LittleFS.begin(true)) {
      Serial.println("ERR: LittleFS mount failed after format");
      return;
    }
  }
  File logFile = LittleFS.open("/battery.csv", FILE_APPEND);
  if (!logFile) {
    Serial.println("WARN: Creating new battery.csv");
    logFile = LittleFS.open("/battery.csv", FILE_WRITE);
    if (!logFile) { Serial.println("ERR: Cannot create battery.csv"); return; }
    logFile.println("timestamp,voltage_v,wifi_rssi,boot_count");
    logFile.close();
    logFile = LittleFS.open("/battery.csv", FILE_APPEND);
    if (!logFile) { Serial.println("ERR: Cannot reopen battery.csv"); return; }
  }
  float voltage = getBatteryVoltage();
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &timeinfo);
    logFile.printf("%s,%.2f,%d,%lu\n", ts, voltage, WiFi.RSSI(), (unsigned long)bootCount);
  } else {
    logFile.printf("--:--,%.2f,%d,%lu\n", voltage, WiFi.RSSI(), (unsigned long)bootCount);
  }
  logFile.close();
}

void dumpCSV(const char* filename) {
  if (!LittleFS.begin(false)) {
    Serial.println("WARN: LittleFS mount failed for dump, retrying...");
    if (!LittleFS.begin(true)) {
      Serial.println("ERR: Cannot mount LittleFS for dump");
      return;
    }
  }
  File f = LittleFS.open(filename, FILE_READ);
  if (!f) {
    Serial.printf("File %s not found\n", filename);
    return;
  }
  Serial.printf("--- %s (%d bytes) ---\n", filename, f.size());
  while (f.available()) {
    Serial.write(f.read());
  }
  Serial.printf("\n--- END %s ---\n", filename);
  f.close();
}

void handleSerialCommand() {
  if (Serial.available() <= 0) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "DUMP_BATTERY") {
    dumpCSV("/battery.csv");
  } else if (cmd == "DUMP_BOOT") {
    dumpCSV("/boot.csv");
  } else if (cmd == "DUMP_ALL") {
    dumpCSV("/battery.csv");
    dumpCSV("/boot.csv");
  }
}

#endif
