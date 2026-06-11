#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "config.h"

float getBatteryVoltage() {
  uint32_t raw = 0;
  uint32_t minRaw = 4095, maxRaw = 0;
  for (int i = 0; i < 10; i++) {
    uint16_t sample = analogRead(BATTERY_PIN);
    raw += sample;
    if (sample < minRaw) minRaw = sample;
    if (sample > maxRaw) maxRaw = sample;
    delay(5);
  }
  raw /= 10;
  if ((maxRaw - minRaw) > 200) return -1.0;
  float adcVoltage = (float)raw / 4095.0 * 3.3;
  if (adcVoltage >= 3.1) return -1.0;
  float voltage = adcVoltage * ADC_CALIBRATION * VOLTAGE_MULTIPLIER;
  return voltage;
}

#endif
