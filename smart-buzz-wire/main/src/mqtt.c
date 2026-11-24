#include "mqtt.h"
#include "config.h"

#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "MQTT connected");
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "MQTT disconnected");
    }
}

void mqtt_start(void)
{
    if (client) return;   /* already started (WiFi reconnect) */

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = DEVICE_ID,
    };

    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);   /* reconnects automatically */
}

void mqtt_send_event(game_event_t event)
{
    if (!client) return;

    /* Example: {"deviceId":"buzzwire-esp32c3","eventType":0}
       The web app sets the timestamp itself. */
    char json[96];
    snprintf(json, sizeof(json), "{\"deviceId\":\"%s\",\"eventType\":%d}",
             DEVICE_ID, (int)event);

    /* enqueue = buffered, so the event is not lost on a short disconnect */
    esp_mqtt_client_enqueue(client, MQTT_GAME_TOPIC, json, 0, 1, 0, true);
    ESP_LOGI(TAG, "Sent: %s", json);
}

void mqtt_send_rfid(const char *number)
{
    if (!client) return;

    /* Example: {"deviceId":"buzzwire-esp32c3","number":"996C075A"} */
    char json[96];
    snprintf(json, sizeof(json), "{\"deviceId\":\"%s\",\"number\":\"%s\"}",
             DEVICE_ID, number);

    esp_mqtt_client_enqueue(client, MQTT_RFID_TOPIC, json, 0, 1, 0, true);
    ESP_LOGI(TAG, "Sent: %s", json);
}
