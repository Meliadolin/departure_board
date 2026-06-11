# Wiring Reference — PID Departure Board

Sorted by ESP32-C3 pin number for easy soldering.

## All Connections

| ESP32-C3 Pin | Connects To |
|--------------|-------------|
| GND | Display GND, TP4056 OUT- |
| 3V3 | Display VCC |
| VIN/5V | TP4056 OUT+ |
| GPIO 1 | Display RES |
| GPIO 3 | Display D/C |
| GPIO 4 | R1+R2 junction (voltage divider) |
| GPIO 5 | Display BUSY |
| GPIO 6 | Display SDA (MOSI) |
| GPIO 7 | Display CS |
| GPIO 10 | Display SCL (SCK) |

## Display Header (WeAct GDEY042T81)

| Display Pin | ESP32-C3 Pin |
|-------------|--------------|
| GND | GND |
| VCC | 3V3 |
| SCL | GPIO 10 |
| SDA | GPIO 6 |
| CS | GPIO 7 |
| D/C | GPIO 3 |
| RES | GPIO 1 |
| BUSY | GPIO 5 |

## TP4056 Module

| TP4056 Pad | Connects To |
|------------|-------------|
| IN+ | USB-C 5V (charging port) |
| IN- | USB-C GND (charging port) |
| BAT+ | Battery + |
| BAT- | Battery - |
| OUT+ | ESP32-C3 VIN/5V |
| OUT- | ESP32-C3 GND |

The TP4056 USB-C port is for charging the battery. The ESP32 USB-C port is for flashing and programming.

## Voltage Divider (two 10K resistors)

| Point | Connects To |
|-------|-------------|
| Battery + | R1 leg 1 |
| R1 leg 2 | GPIO 4 AND R2 leg 1 |
| R2 leg 2 | GND |

## Pins to Avoid

| GPIO | Reason |
|------|--------|
| 2 | Strapping — must be LOW at boot |
| 8 | Onboard RGB LED |
| 9 | Boot button |
| 11–17 | Internal flash |
| 18–19 | USB-JTAG |
| 20–21 | UART |