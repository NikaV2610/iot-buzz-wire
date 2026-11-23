/*
  Smart Buzz Wire - ESP32-C3 SuperMini (ESP-IDF)

  Files:
    include/config.h  WiFi, MQTT and pin settings
    src/game.c        game logic (RFID -> start -> collisions -> stop)
    src/mqtt.c        sends game events as JSON to the web app
    src/wifi.c        WiFi connection
    src/rfid.c        RC522 card reader
    src/buzzer.c      buzzer sounds

  The camera (camera-tracker) sends the positions itself,
  the ESP only sends game events.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "buzzer.h"
#include "game.h"
#include "rfid.h"
#include "wifi.h"

void app_main(void)
{
    /* NVS is needed by the WiFi driver */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    buzzer_init();
    rfid_init();
    game_init();
    wifi_init();   /* MQTT starts automatically once WiFi is connected */

    while (1) {
        game_update();
        buzzer_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
