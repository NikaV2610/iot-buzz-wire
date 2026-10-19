#include "mqtt.h"
#include "config.h"
#include "game.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static volatile bool mqtt_connected = false;
static bool mqtt_started = false;

/* Topics (assembled in mqtt_build_topics()) */
static char T_STATUS[96], T_PLAYER[96], T_START[96], T_END[96],
            T_TOUCH[96], T_ABORT[96], T_BESTRUN[96], T_CMD[96];

static char mqtt_client_id[40];

void mqtt_build_topics(void)
{
    snprintf(T_STATUS,  sizeof(T_STATUS),  "%s/status",    TOPIC_PREFIX);
    snprintf(T_PLAYER,  sizeof(T_PLAYER),  "%s/player",    TOPIC_PREFIX);
    snprintf(T_START,   sizeof(T_START),   "%s/run/start", TOPIC_PREFIX);
    snprintf(T_END,     sizeof(T_END),     "%s/run/end",   TOPIC_PREFIX);
    snprintf(T_TOUCH,   sizeof(T_TOUCH),   "%s/run/touch", TOPIC_PREFIX);
    snprintf(T_ABORT,   sizeof(T_ABORT),   "%s/run/abort", TOPIC_PREFIX);
    snprintf(T_BESTRUN, sizeof(T_BESTRUN), "%s/bestrun",   TOPIC_PREFIX);
    snprintf(T_CMD,     sizeof(T_CMD),     "%s/cmd",       TOPIC_PREFIX);
}

static void mqtt_pub(const char *topic, const char *payload, int qos, int retain)
{
    if (!mqtt_client) return;
    if (qos > 0) {
        /* enqueue also buffers across short connection drops -> result is not lost */
        esp_mqtt_client_enqueue(mqtt_client, topic, payload, 0, qos, retain, true);
    } else if (mqtt_connected) {
        esp_mqtt_client_publish(mqtt_client, topic, payload, 0, qos, retain);
    }
}

void mqtt_publish_player(const char *player_uid)
{
    mqtt_pub(T_PLAYER, player_uid, 1, 0);
}

void mqtt_publish_run_start(uint32_t run_id, const char *player_uid)
{
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"run_id\":%lu,\"player\":\"%s\"}",
             (unsigned long)run_id, player_uid);
    mqtt_pub(T_START, buf, 1, 0);   /* signal for PC (camera) and web app */
}

void mqtt_publish_run_end(uint32_t run_id, const char *player_uid, uint32_t time_ms, uint16_t touches)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lu,\"player\":\"%s\",\"time_ms\":%lu,\"touches\":%u}",
             (unsigned long)run_id, player_uid, (unsigned long)time_ms, touches);
    mqtt_pub(T_END, buf, 1, 0);
}

void mqtt_publish_run_touch(uint32_t t_ms, uint16_t count)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"t_ms\":%lu,\"count\":%u}", (unsigned long)t_ms, count);
    mqtt_pub(T_TOUCH, buf, 0, 0);
}

void mqtt_publish_run_abort(uint32_t run_id)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"run_id\":%lu}", (unsigned long)run_id);
    mqtt_pub(T_ABORT, buf, 1, 0);
}

static void handle_mqtt_message(const char *topic, const char *payload)
{
    if (strcmp(topic, T_BESTRUN) == 0) {
        /* PLACEHOLDER: agree on payload format with the web app, e.g. JSON:
             {"time_ms": 12340, "samples": [[0,0.0],[100,0.01], ...]}
           Currently: payload = time in ms as a number. */
        uint32_t time_ms = (uint32_t)strtoul(payload, NULL, 10);
        game_set_best_run_time(time_ms);
        ESP_LOGI(TAG, "Best run received: %lu ms", (unsigned long)time_ms);
    } else if (strcmp(topic, T_CMD) == 0) {
        /* PLACEHOLDER: commands from the web app */
        if (strcmp(payload, "reset") == 0) {
            game_request_reset();   /* executed in the game loop */
        }
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_connected = true;
        esp_mqtt_client_publish(mqtt_client, T_STATUS, "online", 0, 1, 1);   /* retained */
        esp_mqtt_client_subscribe(mqtt_client, T_BESTRUN, 1);
        esp_mqtt_client_subscribe(mqtt_client, T_CMD, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_DATA: {
        /* topic and data are NOT null-terminated -> copy */
        char topic[128], payload[64];
        int tl = event->topic_len < (int)sizeof(topic) - 1 ? event->topic_len : (int)sizeof(topic) - 1;
        int dl = event->data_len  < (int)sizeof(payload) - 1 ? event->data_len : (int)sizeof(payload) - 1;
        memcpy(topic, event->topic, tl);     topic[tl] = '\0';
        memcpy(payload, event->data, dl);    payload[dl] = '\0';
        handle_mqtt_message(topic, payload);
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;

    default:
        break;
    }
}

void mqtt_start(void)
{
    if (mqtt_started) return;
    mqtt_started = true;

    /* Unique client ID (otherwise two clients kick each other out) */
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "buzzwire-%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = mqtt_client_id,
        /* For brokers with login:
           .credentials.username = "user",
           .credentials.authentication.password = "pass",
           For TLS: .broker.verification.certificate = (const char *)ca_pem_start; */
        .session.last_will = {
            .topic = T_STATUS,
            .msg = "offline",
            .qos = 1,
            .retain = true,
        },
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);   /* reconnects automatically after a drop */
}
