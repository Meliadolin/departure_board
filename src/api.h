#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "config.h"

extern std::vector<String> displayLines;
extern String currentStopName;

struct ApiErrorMsg { String title; String detail; String log; };

ApiErrorMsg getApiErrorMessage(int httpCode) {
  ApiErrorMsg msg;
  msg.log = "API Error: HTTP " + String(httpCode);
  switch(httpCode) {
    case -1:
      msg.title = "Connection Failed";
      msg.detail = "Cannot reach API server";
      msg.log += " - Connection refused or DNS failed";
      break;
    case -11:
      msg.title = "Request Timeout";
      msg.detail = "API took too long (" + String(MAX_API_RETRIES + 1) + " attempts)";
      msg.log += " - Server not responding after retries";
      break;
    case 401:
      msg.title = "Invalid API Key";
      msg.detail = "Check key in config";
      msg.log += " - Authentication failed";
      break;
    case 404:
      msg.title = "Station Not Found";
      msg.detail = "Check station name";
      msg.log += " - Invalid station name or endpoint";
      break;
    case 429:
      msg.title = "Rate Limited";
      msg.detail = "Too many requests";
      msg.log += " - API rate limit exceeded";
      break;
    case 500:
    case 502:
    case 503:
      msg.title = "Server Error";
      msg.detail = "API temporarily down";
      msg.log += " - Server-side issue";
      break;
    default:
      msg.title = "Connection Issue";
      msg.detail = "HTTP " + String(httpCode);
      break;
  }
  return msg;
}

struct HubMapping { const char* input; const char* resolved; };
const HubMapping HUB_MAPPINGS[] = {
  {"Dejvická", "Dejvická,Vítězné náměstí"},
  {"Vítězné náměstí", "Dejvická,Vítězné náměstí"},
  {"Anděl", "Anděl,Na Knížecí"},
  {"Na Knížecí", "Anděl,Na Knížecí"},
};
const int HUB_MAPPING_COUNT = sizeof(HUB_MAPPINGS) / sizeof(HUB_MAPPINGS[0]);

String resolveHubName(String inputName) {
  for (int i = 0; i < HUB_MAPPING_COUNT; i++) {
    if (inputName.equalsIgnoreCase(HUB_MAPPINGS[i].input)) return String(HUB_MAPPINGS[i].resolved);
  }
  return inputName;
}

String urlEncode(String str) {
  String encodedString = "";
  char c, code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%'; encodedString += code0; encodedString += code1;
    }
    yield();
  }
  return encodedString;
}

void updateDisplay();

void getData() {
  esp_task_wdt_reset();

  if (String(PID_API_KEY) == "" || String(PID_API_KEY) == "your-golemio-api-key") {
    displayLines.clear();
    displayLines.push_back("API Key Missing!");
    displayLines.push_back("Set PID_API_KEY in config");
    updateDisplay();
    return;
  }

  int httpCode = -1;
  String payload = "";

  for (int attempt = 0; attempt <= MAX_API_RETRIES; attempt++) {
    if (attempt > 0) {
      Serial.printf("API retry %d/%d\n", attempt, MAX_API_RETRIES);
      delay(2000);
      esp_task_wdt_reset();
    }
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String rawName = resolveHubName(currentStopName);
    String url = "https://api.golemio.cz/v2/pid/departureboards?limit=" + String(API_LIMIT);
    int commaIndex = rawName.indexOf(',');
    if (commaIndex > 0) {
      url += "&names=" + urlEncode(rawName.substring(0, commaIndex));
      url += "&names=" + urlEncode(rawName.substring(commaIndex + 1));
    } else {
      url += "&names=" + urlEncode(rawName);
    }
    http.begin(client, url);
    http.setTimeout(API_TIMEOUT_MS);
    http.addHeader("x-access-token", PID_API_KEY);
    http.addHeader("Content-Type", "application/json; charset=utf-8");
    httpCode = http.GET();
    esp_task_wdt_reset();
    if (httpCode == 200) { payload = http.getString(); http.end(); break; }
    http.end();
    if (attempt < MAX_API_RETRIES) Serial.printf("HTTP %d, retrying...\n", httpCode);
  }

  if (httpCode == 200 && payload.length() > 0) {
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      JsonArray departures = doc["departures"];
      std::vector<String> mLines, tLines, bLines, trLines, fLines, oLines;
      int metroCount = 0, tramCount = 0, busCount = 0, trainCount = 0, ferryCount = 0, otherCount = 0;
      std::vector<String> seenKeys;

      for (JsonObject dep : departures) {
        if (!dep["route"].is<JsonObject>() || !dep["departure_timestamp"].is<JsonObject>()) continue;
        int routeType = dep["route"]["type"] | -1;
        if (routeType < 0) continue;
        String line = dep["route"]["short_name"].as<String>();
        String headsign = dep["trip"]["headsign"].as<String>();
        bool isDiv = dep["trip"]["is_diversion"] | false;
        bool isSub = dep["trip"]["is_substitute_transport"] | false;
        String minutes = dep["departure_timestamp"]["minutes"].as<String>();
        int delayMin = dep["delay"]["minutes"];

        int mode = -1;
        if (line == "A" || line == "B" || line == "C") mode = 1;
        else if (line.length() > 0 && line[0] >= '0' && line[0] <= '9') {
          int num = line.toInt();
          if ((num >= 1 && num <= 26) || (num >= 91 && num <= 99)) mode = 0;
          else if (num >= 100) mode = 3;
        } else if (line.length() > 0) {
          char c = line[0];
          if (c == 'S' || c == 'R' || c == 'E') mode = 2;
        }
        if (mode < 0) {
          if (routeType == 1) mode = 1;
          else if (routeType == 0) mode = 0;
          else if (routeType == 3 || routeType == 11) mode = 3;
          else if (routeType == 2) mode = 2;
          else if (routeType == 4) mode = 4;
        }
        Serial.printf("Line %s routeType=%d -> mode=%d\n", line.c_str(), routeType, mode);

        String dedupKey = line + "|" + headsign;
        bool alreadySeen = false;
        for (auto &k : seenKeys) { if (k == dedupKey) { alreadySeen = true; break; } }
        if (alreadySeen) continue;
        seenKeys.push_back(dedupKey);

        if (mode == 1 && !SHOW_METRO) continue;
        if (mode == 0 && !SHOW_TRAM) continue;
        if (mode == 3 && !SHOW_BUS) continue;

        if (isDiv || isSub) line += "*";
        String entry = line + " -> " + headsign + " (" + minutes + " min)";
        if (delayMin > 0) entry += " [+" + String(delayMin) + "]";
        else if (delayMin < -1) entry += " [Early]";
        entry.trim();

        if (mode == 1 && metroCount < MAX_METRO) { mLines.push_back(entry); metroCount++; }
        else if (mode == 0 && tramCount < MAX_TRAM) { tLines.push_back(entry); tramCount++; }
        else if (mode == 3 && busCount < MAX_BUS) { bLines.push_back(entry); busCount++; }
        else if (mode == 2 && trainCount < MAX_TRAIN) { trLines.push_back(entry); trainCount++; }
        else if (mode == 4 && ferryCount < MAX_FERRY) { fLines.push_back(entry); ferryCount++; }
        else if (mode < 0 && otherCount < MAX_OTHER) { oLines.push_back(entry); otherCount++; }
      }

      displayLines.clear();
      struct tm timeinfo;
      bool timeAppended = false;
      char timeStr[16];
      if (getLocalTime(&timeinfo)) strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
      else strcpy(timeStr, "--:--");

      if (SHOW_HEADER) displayLines.push_back(String(timeStr) + " " + currentStopName);

      auto add_title = [&](const char* title, const std::vector<String>& lines) {
        if (!lines.empty()) {
          String t = title;
          if (!SHOW_HEADER && !timeAppended) { t += "  " + String(timeStr); timeAppended = true; }
          displayLines.push_back(t);
          for (auto l : lines) displayLines.push_back(l);
        }
      };
      add_title("[METRO]", mLines);
      add_title("[TRAIN]", trLines);
      add_title("[TRAM]", tLines);
      add_title("[BUS]", bLines);
      add_title("[FERRY]", fLines);
      add_title("[OTHER]", oLines);

      updateDisplay();
      Serial.println("Board Updated");
    } else {
      Serial.println("JSON Error: " + String(error.c_str()));
      displayLines.clear();
      displayLines.push_back("Data Error");
      displayLines.push_back("JSON: " + String(error.c_str()));
      updateDisplay();
    }
  } else {
    ApiErrorMsg errMsg = getApiErrorMessage(httpCode);
    Serial.println(errMsg.log);
    displayLines.clear();
    displayLines.push_back(errMsg.title);
    displayLines.push_back(errMsg.detail);
    if (httpCode == -11 || httpCode == -1) displayLines.push_back("Check WiFi connection");
    updateDisplay();
  }
}

#endif
