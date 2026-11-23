#pragma once

/* Event numbers must match GameEventType in the web app
   (Hotwire.Web.Web/Iot/Contracts/GameEventMessage.cs) */
typedef enum {
    EVENT_START                = 0,
    EVENT_STOP                 = 1,
    EVENT_COLLISION            = 2,
    EVENT_PLAYER_REGISTRATION  = 3,
    EVENT_RFID_ASSIGNED        = 4,
    EVENT_CALIBRATION_SELECTED = 5,
} game_event_t;

/* Called by wifi.c once the ESP has an IP address */
void mqtt_start(void);

/* Sends one game event as JSON to MQTT_GAME_TOPIC.
   rfid_tag may be "" if no player is known. */
void mqtt_send_event(game_event_t event, const char *rfid_tag);
