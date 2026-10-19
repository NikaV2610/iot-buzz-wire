/*
  Smart Buzz Wire - ESP32 (ESP-IDF)

  Modules:
    config.h     pins, WiFi/MQTT settings
    game.c       state machine and game logic
    debounce.c   input debouncing
    buzzer.c     PWM buzzer
    led_display.c WS2812B best-run display
    rfid.c       RC522 reader
    wifi.c       WiFi station
    mqtt.c       MQTT client and topics

  Required components (see main/idf_component.yml):
    espressif/led_strip  (WS2812B via RMT)
    abobija/rc522        (RC522 via SPI)
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "config.h"
#include "buzzer.h"
#include "game.h"
#include "led_display.h"
#include "mqtt.h"
#include "rfid.h"
#include "wifi.h"

/* PLACEHOLDER: show player time on LED panel / 7-segment display.
   RUNNING: game_elapsed_ms(), FINISHED: run duration */
static void time_display_update(void)
{
}

void app_main(void)
{
    /* NVS is required by the WiFi driver */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    mqtt_build_topics();
    game_init();
    buzzer_init();
    strip_init();
    rfid_init();
    wifi_init();   /* MQTT starts automatically once an IP address is available */

    uint32_t last_display = 0;

    while (1) {
        game_update();
        buzzer_update();

        if (millis() - last_display >= 50) {
            last_display = millis();
            best_run_led_update(game_is_running(), game_elapsed_ms(), game_best_run_time());
            time_display_update();
        }

        vTaskDelay(1);   /* 1 tick = 1 ms at CONFIG_FREERTOS_HZ=1000 (sdkconfig.defaults); pdMS_TO_TICKS(1) would be 0 at 100 Hz -> watchdog */
    }
}
