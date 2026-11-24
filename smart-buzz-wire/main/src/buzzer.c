#include "buzzer.h"
#include "config.h"

#include "esp_err.h"
#include "driver/ledc.h"

/* Buzzer (PWM via LEDC, non-blocking).
   A sound is a list of notes; freq 0 = short pause. */
typedef struct {
    uint16_t freq_hz;
    uint16_t ms;
} note_t;

static const note_t snd_card[]      = { {1000, 70}, {0, 50}, {1000, 70} };
static const note_t snd_start[]     = { {523, 100}, {659, 100}, {784, 180} };          /* C E G */
static const note_t snd_collision[] = { {180, 350} };                                   /* low buzz */
static const note_t snd_stop[]      = { {784, 120}, {988, 120}, {1175, 120}, {1568, 400} };
static const note_t snd_button[]    = { {2500, 40} };

static const note_t *notes = 0;   /* sound that is playing */
static int note_count = 0;
static int note_index = 0;
static uint32_t note_end = 0;

static void tone(uint32_t freq_hz)
{
    if (freq_hz > 0) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);   /* 50 % = on */
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);     /* off */
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void start_note(void)
{
    tone(notes[note_index].freq_hz);
    note_end = millis() + notes[note_index].ms;
}

void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num = PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

#define PLAY(list) do { notes = list; note_count = sizeof(list) / sizeof(list[0]); } while (0)

void buzzer_play(sound_t sound)
{
    switch (sound) {
        case SOUND_CARD:      PLAY(snd_card);      break;
        case SOUND_START:     PLAY(snd_start);     break;
        case SOUND_COLLISION: PLAY(snd_collision); break;
        case SOUND_STOP:      PLAY(snd_stop);      break;
        case SOUND_BUTTON:    PLAY(snd_button);    break;
    }
    note_index = 0;   /* a new sound replaces the old one */
    start_note();
}

void buzzer_update(void)
{
    if (notes == 0 || (int32_t)(millis() - note_end) < 0) {
        return;   /* nothing playing or note not finished yet */
    }

    note_index++;
    if (note_index < note_count) {
        start_note();
    } else {
        tone(0);
        notes = 0;
    }
}
