#include "led_display.h"
#include "config.h"

#include "esp_err.h"
#include "led_strip.h"

/* LED strip (WS2812B via RMT) */
static led_strip_handle_t strip = NULL;

void strip_init(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = PIN_LED_STRIP,
        .max_leds = NUM_LEDS,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags = { .with_dma = false },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));
    led_strip_clear(strip);
}

void best_run_led_update(bool running, uint32_t elapsed_ms, uint32_t best_run_time_ms)
{
    /* PLACEHOLDER: determine position from the best-run time series at the current run time:
         float progress = lookup_best_run_progress(elapsed_ms);  // 0.0 .. 1.0
       Currently simplified: constant speed over the best time. */
    static int last_idx = -2;   /* -1 = all off; only redraw on change */
    int idx = -1;

    if (running && best_run_time_ms > 0) {
        float progress = (float)elapsed_ms / (float)best_run_time_ms;
        if (progress > 1.0f) progress = 1.0f;
        idx = (int)(progress * (NUM_LEDS - 1));
    }
    if (idx == last_idx) return;
    last_idx = idx;

    for (int i = 0; i < NUM_LEDS; i++) {
        led_strip_set_pixel(strip, i, 0, 0, (i == idx) ? 40 : 0);   /* blue, dimmed */
    }
    led_strip_refresh(strip);
}
