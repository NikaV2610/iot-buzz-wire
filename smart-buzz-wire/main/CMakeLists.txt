idf_component_register(
    SRCS "main.c" "game.c" "debounce.c" "buzzer.c" "led_display.c" "rfid.c" "wifi.c" "mqtt.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif mqtt driver esp_timer esp_system
)
