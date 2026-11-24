#pragma once

#include <stdint.h>
#include "esp_timer.h"

/* ============================================================
   WiFi / MQTT (must match the web app: Hotwire.Web appsettings.json)
   ============================================================ */
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASS       "YOUR_PASSWORD"

#define MQTT_URI        "mqtt://broker.emqx.io:1883"
#define MQTT_GAME_TOPIC "hotwire/iot-buzz-wire/test/game"
#define MQTT_RFID_TOPIC "hotwire/iot-buzz-wire/test/rfid"
#define DEVICE_ID       "buzzwire-esp32c3"

/* ============================================================
   Pins ESP32-C3 SuperMini (see docs/Verkabelung.drawio)
   Do not use GPIO 8, 9 (boot pins).
   Start, stop and wire: handle loop on GND, pull-up -> contact = LOW.
   Button modules (blue, red) on 3.3 V (never 5 V!), pull-down -> pressed = HIGH.
   ============================================================ */
#define PIN_WIRE            10    /* course wire */
#define PIN_START_CONTACT   21
#define PIN_STOP_CONTACT    20
#define PIN_CAL_BUTTON      1     /* blue button: calibration */
#define PIN_PLAYER_BUTTON   2     /* red button: new player (scan card) */
#define PIN_BUZZER          0     /* PWM */

#define PIN_RFID_SCK        4
#define PIN_RFID_MISO       5
#define PIN_RFID_MOSI       6
#define PIN_RFID_SS         7
#define PIN_RFID_RST        -1    /* not connected, soft reset */

#define RFID_UID_LEN        24    /* max. UID string length incl. terminator */

/* Milliseconds since boot */
static inline uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}
