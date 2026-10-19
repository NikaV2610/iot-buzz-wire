#pragma once

#include <stdint.h>
#include "esp_timer.h"

/* ============================================================
   Configuration
   ============================================================ */
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASS       "YOUR_PASSWORD"

/* Public broker (for testing). Warning: anyone can read and write! */
#define MQTT_URI        "mqtt://broker.hivemq.com:1883"
/* For TLS e.g. "mqtts://<host>:8883" plus certificate and login (see comment in mqtt_start) */

/* Unique prefix so your topics do not collide with others */
#define TOPIC_PREFIX    "fhv-iot-groupX/buzzwire"   /* <- ADJUST */

/* ============================================================
   Pin assignment (according to block diagram, adjust if needed)
   ============================================================ */
#define PIN_LED_STRIP       4     /* WS2812B data line (best-run display) */
#define PIN_BUZZER          25    /* PWM */
#define PIN_WIRE            27    /* Pull-up, loop connected to GND */
#define PIN_START_CONTACT   32    /* Pull-up */
#define PIN_FINISH_CONTACT  33    /* Pull-up */
#define PIN_BUTTON          26    /* Pull-up (reset/abort) */
#define PIN_TRACKING_LED    16    /* via series resistor (transistor if needed) */

#define PIN_RFID_SCK        18
#define PIN_RFID_MOSI       23
#define PIN_RFID_MISO       19
#define PIN_RFID_SS         5
#define PIN_RFID_RST        22

#define NUM_LEDS            60    /* PLACEHOLDER: length of the best-run strip */

#define RFID_UID_LEN        24    /* max. UID string length incl. terminator */

static inline uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}
