#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define PID_API_KEY "YOUR_GOLEMIO_API_KEY"
#define PID_STOP "YOUR_STOP_NAME"

#define SHOW_METRO true
#define SHOW_TRAM true
#define SHOW_BUS false

#define SHOW_HEADER true
#define INVERT_COLORS false

#define REFRESH_RATE 120

#define LOG_EVERY_N_REFRESHES 1 // Log battery every N refresh cycles
#define FORMAT_LITTLEFS false

#define MAX_METRO 4
#define MAX_TRAM 8
#define MAX_BUS 6
#define MAX_TRAIN 4
#define MAX_FERRY 4
#define MAX_OTHER 4

#define QUIET_HOURS_ENABLED true
#define QH_START 23
#define QH_END 6
#define QH_REFRESH_INTERVAL 15

#define EPD_CS 7
#define EPD_DC 3
#define EPD_RST 1
#define EPD_BUSY 5
#define EPD_SCK 10
#define EPD_MOSI 6
#define BATTERY_PIN 4

const float VOLTAGE_MULTIPLIER = 2.0;
const float ADC_CALIBRATION = 0.91;
const float MIN_BATT_VOLTAGE = 3.3;
const float CHARGED_BATT_VOLTAGE = 4.0;
const int DISPLAY_MARGIN = 15;
const int API_TIMEOUT_MS = 20000;
const int API_LIMIT = 20;
const int MAX_API_RETRIES = 2;
const int WIFI_TIMEOUT_SEC = 15;

#endif
