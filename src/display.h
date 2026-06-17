#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"

extern GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;
extern std::vector<String> displayLines;

void initDisplay() {
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200);
  display.setRotation(0);
  u8g2.begin(display);
}

void displayBootScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    uint16_t bgColor = INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
    uint16_t fgColor = INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
    display.fillScreen(bgColor);
    u8g2.setForegroundColor(fgColor);
    u8g2.setBackgroundColor(bgColor);
    u8g2.setFont(u8g2_font_helvB18_te);
    int tw = u8g2.getUTF8Width("PID BOARD");
    u8g2.drawStr(display.width() / 2 - tw / 2, 40, "PID BOARD");
    u8g2.setFont(u8g2_font_helvB10_te);
    int sw = u8g2.getUTF8Width("Prague Public Transport");
    u8g2.drawStr(display.width() / 2 - sw / 2, 60, "Prague Public Transport");
    display.drawLine(10, 70, display.width() - 10, 70, fgColor);
    u8g2.setFont(u8g2_font_helvR10_te);
    int stw = u8g2.getUTF8Width("Initializing...");
    u8g2.drawStr(display.width() / 2 - stw / 2, 100, "Initializing...");
  } while (display.nextPage());
}

std::vector<String> splitLine(String text, int maxWidth, const uint8_t* font) {
  std::vector<String> result;
  u8g2.setFont(font);
  while (text.length() > 0) {
    int currentWidth = u8g2.getUTF8Width(text.c_str());
    if (currentWidth <= maxWidth) { result.push_back(text); break; }
    int splitPoint = -1;
    for (int i = 0; i < text.length(); ++i) {
      String sub = text.substring(0, i + 1);
      if (u8g2.getUTF8Width(sub.c_str()) > maxWidth) {
        for (int j = i; j >= 0; --j) { if (text.charAt(j) == ' ') { splitPoint = j; break; } }
        if (splitPoint == -1) {
          if (i == 0) { result.push_back(text.substring(0, 1)); text = text.substring(1); }
          else { result.push_back(text.substring(0, i)); text = text.substring(i); }
          text.trim();
        }
        break;
      }
    }
    if (splitPoint != -1) { result.push_back(text.substring(0, splitPoint)); text = text.substring(splitPoint + 1); text.trim(); }
    else if (text.length() > 0 && u8g2.getUTF8Width(text.c_str()) <= maxWidth) { result.push_back(text); text = ""; }
  }
  return result;
}

int countWrappedLines(String text, int maxWidth, const uint8_t* font) {
  int count = 0;
  u8g2.setFont(font);
  while (text.length() > 0) {
    count++;
    if (u8g2.getUTF8Width(text.c_str()) <= maxWidth) break;
    int splitPoint = -1;
    for (int i = 0; i < text.length(); ++i) {
      if (u8g2.getUTF8Width(text.substring(0, i + 1).c_str()) > maxWidth) {
        for (int j = i; j >= 0; --j) { if (text.charAt(j) == ' ') { splitPoint = j; break; } }
        if (splitPoint == -1) { text = (i == 0) ? text.substring(1) : text.substring(i); text.trim(); }
        break;
      }
    }
    if (splitPoint != -1) { text = text.substring(splitPoint + 1); text.trim(); }
    else if (text.length() > 0 && u8g2.getUTF8Width(text.c_str()) <= maxWidth) text = "";
  }
  return count;
}

void drawBatteryIcon(float voltage) {
  uint16_t fgColor = INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
  u8g2.setFont(u8g2_font_helvB08_te);

  if (voltage < 0) {
    int w = u8g2.getUTF8Width("AC");
    u8g2.setCursor(display.width() - w - 5, 10);
    u8g2.print("AC");
  } else {
    const char* label;
    if (voltage >= CHARGED_BATT_VOLTAGE) label = "FULL";
    else if (voltage >= MIN_BATT_VOLTAGE) label = "OK";
    else label = "LOW";
    int w = u8g2.getUTF8Width(label);
    u8g2.setCursor(display.width() - w - 5, 10);
    u8g2.print(label);
    u8g2.setFont(u8g2_font_tom_thumb_4x6_mn);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2fV", voltage);
    int vw = u8g2.getUTF8Width(vbuf);
    u8g2.setCursor(display.width() - vw - 5, 20);
    u8g2.print(vbuf);
  }
}

void updateDisplay() {
  display.setFullWindow();

  const uint8_t* selectedFont = u8g2_font_helvB08_te;
  int lineHeight = 12;

  struct FontOption { const uint8_t* font; int height; };
  FontOption fontOptions[] = {
    {u8g2_font_helvB24_te, 42}, {u8g2_font_helvB18_te, 32}, {u8g2_font_helvB14_te, 25},
    {u8g2_font_helvB12_te, 21}, {u8g2_font_helvB10_te, 18}, {u8g2_font_helvB08_te, 12}
  };
  const uint8_t* headerFont = u8g2_font_helvB14_te;
  const int headerLineHeight = 25;
  int maxDisplayWidth = display.width() - DISPLAY_MARGIN;

  int headerTotalHeight = 0;
  if (SHOW_HEADER && !displayLines.empty()) {
    headerTotalHeight = countWrappedLines(displayLines[0], maxDisplayWidth, headerFont) * headerLineHeight + 6;
  }

  std::vector<String> contentLines;
  size_t startContentIndex = (SHOW_HEADER && !displayLines.empty()) ? 1 : 0;
  for (size_t i = startContentIndex; i < displayLines.size(); ++i) contentLines.push_back(displayLines[i]);

  for (size_t i = 0; i < sizeof(fontOptions) / sizeof(fontOptions[0]); ++i) {
    int totalWrapped = 0;
    for (const String& line : contentLines) totalWrapped += countWrappedLines(line, maxDisplayWidth, fontOptions[i].font);
    if (headerTotalHeight + (totalWrapped * fontOptions[i].height) <= display.height()) {
      selectedFont = fontOptions[i].font; lineHeight = fontOptions[i].height; break;
    }
  }

  uint16_t bgColor = INVERT_COLORS ? GxEPD_BLACK : GxEPD_WHITE;
  uint16_t fgColor = INVERT_COLORS ? GxEPD_WHITE : GxEPD_BLACK;
  float batteryVoltage = getBatteryVoltage();

  display.firstPage();
  do {
    display.fillScreen(bgColor);
    u8g2.setForegroundColor(fgColor);
    u8g2.setBackgroundColor(bgColor);
    u8g2.setFont(selectedFont);
    drawBatteryIcon(batteryVoltage);

    if (SHOW_HEADER && !displayLines.empty()) {
      u8g2.setFont(headerFont);
      int currentY = u8g2.getFontAscent();
      std::vector<String> wrappedHeader = splitLine(displayLines[0], display.width() - DISPLAY_MARGIN, headerFont);
      for (const String& line : wrappedHeader) { u8g2.setCursor(5, currentY); u8g2.print(line); currentY += headerLineHeight; }
      u8g2.setFont(selectedFont);
      int startY;
      if (!contentLines.empty()) {
        int separatorY = (currentY - headerLineHeight) + 8;
        display.drawFastHLine(0, separatorY, display.width(), fgColor);
        startY = separatorY + 4 + u8g2.getFontAscent();
      } else {
        startY = currentY;
      }
      for (const String& line : contentLines) {
        if (startY > display.height()) break;
        std::vector<String> wrapped = splitLine(line, display.width() - DISPLAY_MARGIN, selectedFont);
        for (const String& w : wrapped) { if (startY > display.height()) break; u8g2.setCursor(5, startY); u8g2.print(w); startY += lineHeight; }
      }
    } else {
      int y = 24 + lineHeight;
      for (size_t i = 0; i < displayLines.size(); i++) {
        if (y > display.height()) break;
        std::vector<String> wrapped = splitLine(displayLines[i], display.width() - DISPLAY_MARGIN, selectedFont);
        for (const String& line : wrapped) { if (y > display.height()) break; u8g2.setCursor(5, y); u8g2.print(line); y += lineHeight; }
      }
    }
  } while (display.nextPage());
}

#endif
