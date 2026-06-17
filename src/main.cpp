#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <vector>
#include <time.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "logger.h"
#include "battery.h"
#include "wifi_manager.h"
#include "display.h"
#include "api.h"

GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2;
std::vector<String> displayLines;
String currentStopName = PID_STOP;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n\n=== PID BOARD STARTING ===");
  Serial.flush();

  delay(500);
  initLogger();
  float voltage = getBatteryVoltage();
  if (voltage < 0) {
    Serial.println("No battery detected (floating ADC) - Always-On mode");
  } else {
    Serial.printf("Battery voltage: %.2fV - Always-On mode\n", voltage);
  }
  Serial.flush();

  initDisplay();
  displayBootScreen();
  connectWiFiAndTime();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi failed, sleeping...");
    uint32_t sleepSeconds = isQuietHour() ? calculateQuietHoursSleepDuration() : REFRESH_RATE;
    displayLines.clear();
    displayLines.push_back("WiFi Connection Failed");
    displayLines.push_back("Check SSID/password in config");
    displayLines.push_back("Retrying in " + String(sleepSeconds) + "s...");
    updateDisplay();
    display.powerOff();
    esp_deep_sleep(sleepSeconds * 1000000ULL);
  }

  if (isQuietHour()) {
    getData();
    uint32_t sleepSeconds = calculateQuietHoursSleepDuration();
    Serial.printf("Quiet hours. Sleeping %d seconds.\n", sleepSeconds);
    WiFi.disconnect(false);
    display.powerOff();
    esp_deep_sleep(sleepSeconds * 1000000ULL);
  }

  getData();
}

void loop() {
  static bool firstLoop = true;
  static unsigned long lastCheck = 0;
  static unsigned long lastReconnect = 0;

  if (firstLoop) {
    delay(1000);
    Serial.println("\n\n*** LOOP STARTED - Always-On mode ***");
    Serial.println("Commands: DUMP_BATTERY, DUMP_BOOT, DUMP_ALL");
    Serial.flush();
    firstLoop = false;
  }

  handleSerialCommand();
  esp_task_wdt_reset();

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > 10000 || lastReconnect == 0) {
      Serial.println("WiFi reconnecting...");
      WiFi.disconnect(false);
      delay(500);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        esp_task_wdt_reset();
        attempts++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected!");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();
      }
      lastReconnect = millis();
    }
    delay(1000);
    esp_task_wdt_reset();
    return;
  }

  if (isQuietHour()) {
    getData();
    uint32_t sleepSeconds = calculateQuietHoursSleepDuration();
    Serial.printf("Quiet hours. Sleeping %d seconds.\n", sleepSeconds);
    WiFi.disconnect(false);
    display.powerOff();
    esp_deep_sleep(sleepSeconds * 1000000ULL);
  }

  if (millis() - lastCheck > (REFRESH_RATE * 1000) || lastCheck == 0) {
    getData();
    static uint32_t refreshCounter = 0;
    if (++refreshCounter >= LOG_EVERY_N_REFRESHES) {
      logBatteryData();
      refreshCounter = 0;
    }
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1000)) {
      int secsIntoMinute = timeinfo.tm_sec;
      lastCheck = millis() - (secsIntoMinute * 1000UL);
    } else {
      lastCheck = millis();
    }
    Serial.printf("Refresh done. Batt: %.2fV | RSSI: %ddBm\n", getBatteryVoltage(), WiFi.RSSI());
    Serial.flush();
  }

  uint32_t timeUntilNext = (lastCheck + (REFRESH_RATE * 1000)) - millis();
  if (timeUntilNext > 1000 && timeUntilNext < (REFRESH_RATE * 1000)) {
    Serial.printf("Light sleep %dms\n", timeUntilNext);
    Serial.flush();
    WiFi.disconnect(false);
    esp_sleep_enable_timer_wakeup(timeUntilNext * 1000ULL);
    esp_light_sleep_start();
  }
}
