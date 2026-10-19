/*
  Game logic (state machine):
    IDLE     -> waits for RFID tag
    READY    -> player known, waits for start contact
    RUNNING  -> timer runs, wire touches are counted
    FINISHED -> result sent, back to IDLE after a short time
*/

#include "game.h"
#include "config.h"
#include "buzzer.h"
#include "debounce.h"
#include "mqtt.h"
#include "rfid.h"

#include <string.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "game";

typedef enum { STATE_IDLE, STATE_READY, STATE_RUNNING, STATE_FINISHED } game_state_t;

static game_state_t state = STATE_IDLE;
static char player_uid[RFID_UID_LEN] = "";
static uint32_t run_id = 0;
static uint32_t run_start_ms = 0;
static uint32_t run_duration_ms = 0;
static uint16_t touch_count = 0;
static uint32_t finished_at_ms = 0;
static uint32_t ready_since_ms = 0;

/* Best run (comes from the web app via MQTT) */
static volatile uint32_t best_run_time_ms = 0;   /* 0 = no best run yet */
/* Set by the MQTT task, evaluated in the game loop (avoids races on state) */
static volatile bool reset_requested = false;
/* PLACEHOLDER: store the best run's time series (time -> progress) here later */

static debounced_t db_wire   = { PIN_WIRE,           false, false, 0, 5  };
static debounced_t db_start  = { PIN_START_CONTACT,  false, false, 0, 10 };
static debounced_t db_finish = { PIN_FINISH_CONTACT, false, false, 0, 10 };
static debounced_t db_button = { PIN_BUTTON,         false, false, 0, 30 };

/* ============================================================
   GPIO
   ============================================================ */
static void gpio_setup(void)
{
    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << PIN_WIRE) | (1ULL << PIN_START_CONTACT) |
                        (1ULL << PIN_FINISH_CONTACT) | (1ULL << PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&in_cfg));

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << PIN_TRACKING_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out_cfg));
    gpio_set_level(PIN_TRACKING_LED, 0);
}

/* ============================================================
   Run control
   ============================================================ */
static void start_run(void)
{
    run_id++;
    run_start_ms = millis();
    touch_count = 0;
    state = STATE_RUNNING;
    gpio_set_level(PIN_TRACKING_LED, 1);   /* marker for the camera on */

    mqtt_publish_run_start(run_id, player_uid);
    beep(1500, 100);
}

static void finish_run(void)
{
    run_duration_ms = millis() - run_start_ms;
    state = STATE_FINISHED;
    finished_at_ms = millis();
    gpio_set_level(PIN_TRACKING_LED, 0);

    mqtt_publish_run_end(run_id, player_uid, run_duration_ms, touch_count);
    beep(2000, 400);
}

static void abort_run(void)
{
    if (state == STATE_RUNNING) {
        /* Inform camera/web app, otherwise tracking keeps running */
        mqtt_publish_run_abort(run_id);
    }
    gpio_set_level(PIN_TRACKING_LED, 0);
    state = STATE_IDLE;
    player_uid[0] = '\0';
}

/* ============================================================
   Input handlers
   ============================================================ */
static void handle_rfid(void)
{
    char uid[RFID_UID_LEN];
    if (!rfid_get_uid(uid)) return;
    if (state != STATE_IDLE) return;   /* only register while idle */

    strncpy(player_uid, uid, sizeof(player_uid) - 1);
    player_uid[sizeof(player_uid) - 1] = '\0';
    mqtt_publish_player(player_uid);
    state = STATE_READY;
    ready_since_ms = millis();
    beep(1000, 150);
    ESP_LOGI(TAG, "Player: %s", player_uid);
}

static void handle_wire(void)
{
    if (debounce_update(&db_wire) && state == STATE_RUNNING) {
        touch_count++;
        beep(400, 150);
        /* PLACEHOLDER: optional time penalty, lives system, end run after X touches ... */
        mqtt_publish_run_touch(millis() - run_start_ms, touch_count);
    }
}

static void handle_contacts(void)
{
    bool start_hit  = debounce_update(&db_start);
    bool finish_hit = debounce_update(&db_finish);

    if (state == STATE_READY && start_hit)     start_run();
    if (state == STATE_RUNNING && finish_hit)  finish_run();
}

static void handle_button(void)
{
    if (debounce_update(&db_button)) {
        /* PLACEHOLDER: distinguish short/long press */
        abort_run();
        beep(600, 100);
    }
}

/* ============================================================
   Status
   ============================================================ */
static void status_update(void)
{
    if (reset_requested) {
        reset_requested = false;
        abort_run();
    }
    /* Nobody starts? Back to idle after 60 s */
    if (state == STATE_READY && (millis() - ready_since_ms) > 60000) {
        state = STATE_IDLE;
        player_uid[0] = '\0';
    }
    /* PLACEHOLDER: status display (colors/text) depending on state
       IDLE: "Scan card", READY: "Ready", RUNNING: time, FINISHED: result */
    if (state == STATE_FINISHED && (millis() - finished_at_ms) > 5000) {
        state = STATE_IDLE;
        player_uid[0] = '\0';
    }
}

/* ============================================================
   Public API
   ============================================================ */
void game_init(void)
{
    gpio_setup();
}

void game_update(void)
{
    handle_button();
    handle_rfid();
    handle_contacts();
    handle_wire();
    status_update();
}

void game_set_best_run_time(uint32_t time_ms)
{
    best_run_time_ms = time_ms;
}

void game_request_reset(void)
{
    reset_requested = true;
}

int game_is_running(void)
{
    return state == STATE_RUNNING;
}

uint32_t game_elapsed_ms(void)
{
    return millis() - run_start_ms;
}

uint32_t game_best_run_time(void)
{
    return best_run_time_ms;
}
