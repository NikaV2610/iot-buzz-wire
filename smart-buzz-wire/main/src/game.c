/*
  Game logic (state machine):
    IDLE    -> waits for an RFID card
    READY   -> player known, waits for the start contact
    RUNNING -> wire touches are counted, stop contact ends the run

  Each step sends one event via MQTT, the web app measures time
  and stores the result.
*/

#include "game.h"
#include "config.h"
#include "buzzer.h"
#include "mqtt.h"
#include "rfid.h"

#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "game";

#define LOCK_MS  300   /* ignore bouncing / scraping right after a hit */

typedef enum { STATE_IDLE, STATE_READY, STATE_RUNNING } game_state_t;

typedef struct {
    gpio_num_t pin;
    bool was_low;
    uint32_t last_hit;
} input_t;

static game_state_t state = STATE_IDLE;
static char player_uid[RFID_UID_LEN] = "";

static input_t in_wire   = { .pin = PIN_WIRE };
static input_t in_start  = { .pin = PIN_START_CONTACT };
static input_t in_stop   = { .pin = PIN_STOP_CONTACT };
static input_t in_button = { .pin = PIN_CAL_BUTTON };

/* Returns true once when the contact closes (pin goes LOW) */
static bool input_hit(input_t *in)
{
    bool low = (gpio_get_level(in->pin) == 0);
    bool hit = low && !in->was_low && (millis() - in->last_hit) > LOCK_MS;

    in->was_low = low;
    if (hit) in->last_hit = millis();
    return hit;
}

void game_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PIN_WIRE) | (1ULL << PIN_START_CONTACT) |
                        (1ULL << PIN_STOP_CONTACT) | (1ULL << PIN_CAL_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

void game_update(void)
{
    /* Read all inputs every time (keeps was_low up to date) */
    bool wire   = input_hit(&in_wire);
    bool start  = input_hit(&in_start);
    bool stop   = input_hit(&in_stop);
    bool button = input_hit(&in_button);

    /* 1) RFID card -> player is ready (only while idle) */
    char uid[RFID_UID_LEN];
    if (rfid_get_uid(uid) && state == STATE_IDLE) {
        strcpy(player_uid, uid);
        state = STATE_READY;
        mqtt_send_event(EVENT_RFID_ASSIGNED, player_uid);
        beep(1000, 150);
        ESP_LOGI(TAG, "Player card: %s", player_uid);
    }

    /* 2) Start contact -> run starts */
    if (start && state == STATE_READY) {
        state = STATE_RUNNING;
        mqtt_send_event(EVENT_START, player_uid);
        beep(1500, 100);
        ESP_LOGI(TAG, "Run started");
    }

    /* 3) Wire touched -> collision */
    if (wire && state == STATE_RUNNING) {
        mqtt_send_event(EVENT_COLLISION, player_uid);
        beep(400, 150);
        ESP_LOGI(TAG, "Collision");
    }

    /* 4) Stop contact -> run finished, back to idle */
    if (stop && state == STATE_RUNNING) {
        state = STATE_IDLE;
        mqtt_send_event(EVENT_STOP, player_uid);
        beep(2000, 400);
        ESP_LOGI(TAG, "Run finished");
        player_uid[0] = '\0';
    }

    /* 5) Calibration button -> tell the web app */
    if (button) {
        mqtt_send_event(EVENT_CALIBRATION_SELECTED, "");
        beep(800, 100);
        ESP_LOGI(TAG, "Calibration button");
    }
}
