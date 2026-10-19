#include "buzzer.h"
#include "config.h"

#include "esp_err.h"
#include "driver/ledc.h"

/* Buzzer (PWM via LEDC, non-blocking) */
static uint32_t buzzer_off_at = 0;

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

void beep(uint32_t freq_hz, uint32_t duration_ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);   /* 50 % */
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    buzzer_off_at = millis() + duration_ms;
}

void buzzer_update(void)
{
    if (buzzer_off_at && (int32_t)(millis() - buzzer_off_at) >= 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        buzzer_off_at = 0;
    }
}
