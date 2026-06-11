# Departure Board

A little e-paper display that hangs on the wall and shows when the next tram/metro/bus is coming to your stop. No apps, no web interface, no cloud dashboard. Just departures.

Built for Prague's PID system using the [Golemio API](https://api.golemio.cz/), but the API code could be adapted for any city with a similar transit feed.

## What it looks like

```
14:32 Hradčanská
─────────────────────────────────
[TRAM]
9  -> Anděl (3 min)
22 -> I.P.Pavlova (5 min)
1  -> Vozovna Pankrác (8 min)
[METRO]
A  -> Depo Hostivař (2 min)
A  -> Nemocnice Motol (6 min)
```

The display auto-scales the font based on how many departures are showing. At night when there's fewer trams, the text gets bigger. During rush hour it shrinks to fit everything.

## Parts List

| Part | Specification | Supplier | Price |
|------|---------------|----------|-------|
| ESP32-C3 SuperMini | ESP32-C3FN4 with 4MB flash | AliExpress | ~€3 |
| WeAct 4.2" e-paper display | GDEY042T81, 400x300, SPI interface | AliExpress | ~€18 |
| TP4056 USB-C charger module | With battery protection circuit | AliExpress | ~€1 |
| 603035 LiPo battery | 700mAh, 3.7V with JST connector | AliExpress/electronics store | ~€5 |
| 2x 10K resistors | 1/4W, 5% tolerance | Electronics store | ~€0.10 |
| Jumper wires | Female-to-female, 10cm | Electronics store | ~€2 |
| 3D printed case (optional) | PLA, 0.2mm layer height | Print yourself | ~€1 |

**Total cost: ~€30**

## Case

The enclosure design is still in progress. The case will house the ESP32, TP4056 module, and battery, with openings for the e-paper display and USB-C charging port.

**Status:** To be added

## Wiring

See [WIRING.md](WIRING.md) for the complete connection reference, sorted by pin number.

Quick summary — 10 wires total:

| From | To |
|------|----|
| ESP32 3V3 | Display VCC |
| ESP32 GND | Display GND, TP4056 OUT- |
| ESP32 VIN/5V | TP4056 OUT+ |
| ESP32 GPIO 1 | Display RES |
| ESP32 GPIO 3 | Display D/C |
| ESP32 GPIO 4 | Voltage divider midpoint |
| ESP32 GPIO 5 | Display BUSY |
| ESP32 GPIO 6 | Display SDA (MOSI) |
| ESP32 GPIO 7 | Display CS |
| ESP32 GPIO 10 | Display SCL (SCK) |

Plus: TP4056 BAT+ → Battery +, TP4056 BAT- → Battery -, TP4056 IN+ → USB-C 5V (charging port), TP4056 IN- → USB-C GND (charging port), and two 10K resistors forming a voltage divider at GPIO4 (Battery+ → 10K → GPIO4 → 10K → GND).

Two USB-C ports are exposed on the case: the TP4056 port for charging the battery, and the ESP32 port for flashing.

## Installation

### 1. Install PlatformIO

You need VS Code with the PlatformIO extension. Or use the PlatformIO CLI if you're a terminal person.

### 2. Clone and open

```bash
git clone <this-repo>
cd departure_board
code .
```

### 3. Edit the config

Open `src/config.h` and change these:

```cpp
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"
#define PID_API_KEY "your-api-key-here"
#define PID_STOP "YourStopName"
```

See [Configuration](#configuration) below for all the options.

### 4. Get an API key

Go to [api.golemio.cz/api-key](https://api.golemio.cz/api-key), sign up, and create a free API key. It takes 2 minutes.

### 5. Flash it

Connect the ESP32-C3 via USB, then:

```bash
pio run -t upload
```

Or click the upload button in PlatformIO (right arrow icon at the bottom).

**Note:** The ESP32-C3 SuperMini has a quirk where you need to hold the BOOT button while pressing RESET to get into upload mode. If the upload fails, try that.

## Configuration

All settings are in `src/config.h`. Change them and reflash.

### Required

| Setting | What it does |
|---------|--------------|
| `WIFI_SSID` | Your WiFi network name |
| `WIFI_PASSWORD` | Your WiFi password |
| `PID_API_KEY` | Golemio API key (free) |
| `PID_STOP` | Stop name as it appears in PID (e.g. "Hradčanská", "Anděl") |

### Display

| Setting | Default | What it does |
|---------|---------|--------------|
| `SHOW_HEADER` | `true` | Show time + stop name at the top |
| `INVERT_COLORS` | `false` | White text on black background |

### Transport types

Toggle which transport types to show:

| Setting | Default | What it shows |
|---------|---------|---------------|
| `SHOW_METRO` | `true` | Metro lines (A, B, C) |
| `SHOW_TRAM` | `true` | Trams (numbered 1-99) |
| `SHOW_BUS` | `false` | Buses (numbered 100+) |

Trains and ferries are always shown if they exist at your stop.

### Limits

How many departures per transport type to show:

```cpp
#define MAX_METRO 4
#define MAX_TRAM 8
#define MAX_BUS 6
#define MAX_TRAIN 4
#define MAX_FERRY 4
#define MAX_OTHER 4
```

### Timing

| Setting | Default | What it does |
|---------|---------|--------------|
| `REFRESH_RATE` | `60` | Seconds between display updates |
| `LOG_EVERY_N_REFRESHES` | `1` | Log battery + WiFi every N refreshes (1 = every 60s, 6 = every 6 minutes) |

### Quiet hours

Stop updating during night hours to save battery:

```cpp
#define QUIET_HOURS_ENABLED false
#define QH_START 23  // 11 PM
#define QH_END 6     // 6 AM
#define QH_REFRESH_INTERVAL 15  // Check every 15 minutes during quiet hours
```

When quiet hours are active, the last timetable stays on the display. The board wakes up periodically to check if quiet hours are over, goes back to sleep if not.

### Battery monitoring

The code automatically detects battery voltage via GPIO4 and shows it in the top-right corner:

- `AC` — plugged in, no battery (or battery disconnected)
- `FULL` — battery ≥ 4.0V (charging or fully charged)
- `OK` — battery 3.3V–4.0V
- `LOW` — battery < 3.3V (change it soon)

The actual voltage is shown below the label.

ESP32-C3's ADC reads about 9.5% high. `ADC_CALIBRATION` in config.h compensates for that — tweak it if your readings are off.

## Logging

The board logs battery voltage, WiFi signal strength, and boot events to the internal flash. The data is stored as CSV files on a dedicated LittleFS partition.

### What gets logged

- **Battery**: `battery.csv` — timestamp, voltage (V), WiFi RSSI (dBm), boot count
- **Boots**: `boot.csv` — timestamp, boot count

One entry per refresh cycle (every `REFRESH_RATE` seconds by default). Change `LOG_EVERY_N_REFRESHES` to log less often.

### Reading the logs

There's two ways to get the data:

**1. Via esptool (recommended)**

Plug the board into USB and run:

```bash
python pull_logs.py /dev/ttyUSB0
```

This dumps the log partition from flash and extracts `battery_log.csv` and `boot_log.csv` to your current folder. Requires esptool — `pip install esptool`.

**2. Via serial**

Connect with a serial monitor at 115200 baud and type `DUMP_BATTERY` or `DUMP_ALL`. This sends the raw CSV over the serial connection. Works on some boards, but the ESP32-C3's USB CDC has a quirk where opening the serial port resets the chip, so it can be finicky.

### FORMAT_LITTLEFS

| Setting | Behavior |
|---------|----------|
| `true` | Wipes all logs on every boot. Useful for testing. |
| `false` | Preserves logs across reboots. Use this for normal operation and battery drain tests. |

If you need to wipe the logs without reflashing, just set this to `true`, flash once, then set it back to `false` and flash again. Or use `pio run -t erase` for a full wipe.

## Power

**Wall power (USB):** Always-on mode. The board stays awake and refreshes every `REFRESH_RATE` seconds. Draws ~30mA idle.

**Battery power:** Deep sleep between refresh cycles. Wakes up, fetches data, updates display, goes back to sleep. Draws ~5µA sleeping, ~80mA for ~5 seconds during refresh.

A 700mAh battery will last about 2-3 days with 60-second refresh rate. Expect longer if you enable quiet hours.

## Known issues

- **Serial monitor isn't reliable** on the ESP32-C3 SuperMini. Opening a serial connection resets the chip, and the TinyUSB re-enumeration drops the host connection after boot. Use `pull_logs.py` to read logs instead.
- **ADC readings are ~9.5% high** on the ESP32-C3. The code applies a calibration factor (0.91) but it's not perfect. The voltage shown is approximate.
- **WiFi on battery cold boot** can be flaky on the ESP32-C3. The board retries up to 3 times with 15-second timeouts. If it still fails, it sleeps and retries next cycle.

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0)**.

You are free to:
- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

Under the following terms:
- **Attribution** — You must give appropriate credit, provide a link to the license, and indicate if changes were made
- **NonCommercial** — You may not use the material for commercial purposes

For commercial use, please contact the author for permission.

Full license text: https://creativecommons.org/licenses/by-nc/4.0/
