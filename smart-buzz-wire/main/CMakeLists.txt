idf_component_register(
    SRCS
        "main.c"
        "src/game.c"
        "src/buzzer.c"
        "src/rfid.c"
        "src/wifi.c"
        "src/mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_wifi esp_event esp_netif mqtt driver esp_timer
)
