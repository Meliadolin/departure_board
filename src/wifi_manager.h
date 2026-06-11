#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "config.h"

void setupTimeZone() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  delay(1000);
}

void connectWiFiAndTime() {
  const int MAX_RETRIES = 3;
  for (int retry = 0; retry < MAX_RETRIES; retry++) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi (attempt %d/%d)...", retry + 1, MAX_RETRIES);
    Serial.flush();
    byte wifi_tries = 0;
    while (WiFi.status() != WL_CONNECTED) {
      wifi_tries++;
      if (wifi_tries > (WIFI_TIMEOUT_SEC * 2)) {
        Serial.println(" Failed.");
        Serial.flush();
        break;
      }
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" Connected!");
      Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
      Serial.flush();
      setupTimeZone();
      return;
    }
    if (retry < MAX_RETRIES - 1) {
      Serial.printf("Retrying in 2s...\n");
      delay(2000);
    }
  }
  Serial.println("WiFi failed after all retries.");
  Serial.flush();
}

bool isQuietHour() {
  if (!QUIET_HOURS_ENABLED) return false;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false;
  int currentHour = timeinfo.tm_hour;
  if (QH_START > QH_END) return (currentHour >= QH_START || currentHour < QH_END);
  else return (currentHour >= QH_START && currentHour < QH_END);
}

uint32_t calculateQuietHoursSleepDuration() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) return QH_REFRESH_INTERVAL * 60;
  int currentMinute = timeinfo.tm_min;
  int currentSecond = timeinfo.tm_sec;
  int minutesSinceBoundary = currentMinute % QH_REFRESH_INTERVAL;
  if (minutesSinceBoundary == 0 && currentSecond == 0) return QH_REFRESH_INTERVAL * 60;
  int secondsToNextBoundary = (QH_REFRESH_INTERVAL - minutesSinceBoundary) * 60 - currentSecond;
  if (secondsToNextBoundary < 60) secondsToNextBoundary = QH_REFRESH_INTERVAL * 60;
  return secondsToNextBoundary;
}

#endif
