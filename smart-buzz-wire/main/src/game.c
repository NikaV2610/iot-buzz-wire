/*
  Game logic:
    Red button    -> "new player": the next scanned card is the player
    Start contact -> errors = 0, counting starts
    Course wire   -> errors + 1 (only while counting)
    Stop contact  -> counting stops, result is shown
    Blue button   -> calibration: start -> follow the wire -> stop,
                     the web app stores this path as the new track

  The same player can play again: just touch the start contact again.
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

typedef enum {
    STATE_NO_PLAYER,   /* press the red button first */
    STATE_SCAN,        /* waiting for a card */
    STATE_READY,       /* player known, touch start */
    STATE_RUNNING,     /* counting errors */
    STATE_CAL_WAIT,    /* blue button pressed, waiting for start */
    STATE_CAL_RUN,     /* calibration run: web app records the path */
} game_state_t;

typedef struct {
    gpio_num_t pin;
    bool active_high;   /* true: contact = 3.3 V, false: contact = GND */
    bool was_on;
    uint32_t last_hit;
} input_t;

static game_state_t state = STATE_NO_PLAYER;
static char player_uid[RFID_UID_LEN] = "";
static int errors = 0;

/* Handle loop is on GND -> contacts pull the pin LOW.
   The button modules switch 3.3 V -> pin goes HIGH. */
static input_t in_wire   = { .pin = PIN_WIRE,          .active_high = false };
static input_t in_start  = { .pin = PIN_START_CONTACT, .active_high = false };
static input_t in_stop   = { .pin = PIN_STOP_CONTACT,  .active_high = false };
static input_t in_cal    = { .pin = PIN_CAL_BUTTON,    .active_high = true  };
static input_t in_player = { .pin = PIN_PLAYER_BUTTON, .active_high = true  };

/* Returns true once when the contact closes */
static bool input_hit(input_t *in)
{
    bool on = (gpio_get_level(in->pin) == (in->active_high ? 1 : 0));
    bool hit = on && !in->was_on && (millis() - in->last_hit) > LOCK_MS;

    in->was_on = on;
    if (hit) in->last_hit = millis();
    return hit;
}

/* Pull-up for GND contacts, pull-down for 3.3 V contacts */
static void input_setup(input_t *in)
{
    gpio_reset_pin(in->pin);
    gpio_set_direction(in->pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(in->pin, in->active_high ? GPIO_PULLDOWN_ONLY : GPIO_PULLUP_ONLY);
}

void game_init(void)
{
    input_setup(&in_wire);
    input_setup(&in_start);
    input_setup(&in_stop);
    input_setup(&in_cal);
    input_setup(&in_player);

    ESP_LOGI(TAG, "Press the red button to register a player");
}

void game_update(void)
{
    /* Read all inputs every time (keeps was_on up to date) */
    bool wire   = input_hit(&in_wire);
    bool start  = input_hit(&in_start);
    bool stop   = input_hit(&in_stop);
    bool cal    = input_hit(&in_cal);
    bool player = input_hit(&in_player);

    char uid[RFID_UID_LEN];
    bool card = rfid_get_uid(uid);   /* always read, so old cards are dropped */

    /* 1) Red button -> new player, wait for a card */
    if (player && state != STATE_RUNNING && state != STATE_CAL_RUN) {
        state = STATE_SCAN;
        mqtt_send_event(EVENT_PLAYER_REGISTRATION);   /* "newPlayer" */
        buzzer_play(SOUND_BUTTON);
        ESP_LOGI(TAG, "New player: hold the card to the reader");
    }

    /* 2) Card -> player is ready (only after the red button) */
    if (card && state == STATE_SCAN) {
        strcpy(player_uid, uid);
        state = STATE_READY;
        mqtt_send_rfid(player_uid);   /* own RFID topic */
        buzzer_play(SOUND_CARD);
        ESP_LOGI(TAG, "Player %s ready - touch the start contact", player_uid);
    }

    /* 3) Start contact -> errors = 0, counting starts
          (also during a run: back to start = restart) */
    if (start) {
        if (state == STATE_READY || state == STATE_RUNNING) {
            errors = 0;
            state = STATE_RUNNING;
            mqtt_send_event(EVENT_START);
            buzzer_play(SOUND_START);
            ESP_LOGI(TAG, "START - errors: 0");
        } else if (state == STATE_CAL_WAIT) {
            state = STATE_CAL_RUN;
            mqtt_send_event(EVENT_CALIBRATION_SELECTED);   /* web app starts recording */
            buzzer_play(SOUND_START);
            ESP_LOGI(TAG, "CALIBRATION - follow the wire slowly to the stop contact");
        } else if (state == STATE_NO_PLAYER || state == STATE_SCAN) {
            ESP_LOGW(TAG, "No player - press the red button and scan a card first");
        }
    }

    /* 4) Wire touched -> errors + 1 */
    if (wire && state == STATE_RUNNING) {
        errors++;
        mqtt_send_event(EVENT_COLLISION);
        buzzer_play(SOUND_COLLISION);
        ESP_LOGI(TAG, "Error! errors: %d", errors);
    }

    /* 5) Stop contact -> counting stops */
    if (stop && state == STATE_RUNNING) {
        state = STATE_READY;   /* same player can start again */
        mqtt_send_event(EVENT_STOP);
        buzzer_play(SOUND_STOP);
        ESP_LOGI(TAG, "FINISH - player %s, errors: %d", player_uid, errors);
    }

    /* 5b) Stop contact during calibration -> web app saves the new track */
    if (stop && state == STATE_CAL_RUN) {
        state = player_uid[0] ? STATE_READY : STATE_NO_PLAYER;
        mqtt_send_event(EVENT_STOP);
        buzzer_play(SOUND_STOP);
        ESP_LOGI(TAG, "CALIBRATION DONE - track saved in the web app (reload the Track page)");
    }

    /* 6) Blue button -> calibration mode on/off (not during a run) */
    if (cal && state != STATE_RUNNING && state != STATE_CAL_RUN) {
        buzzer_play(SOUND_BUTTON);
        if (state == STATE_CAL_WAIT) {
            state = player_uid[0] ? STATE_READY : STATE_NO_PLAYER;
            ESP_LOGI(TAG, "Calibration cancelled");
        } else {
            state = STATE_CAL_WAIT;
            ESP_LOGI(TAG, "Calibration: touch start, follow the wire, touch stop");
        }
    }
}
