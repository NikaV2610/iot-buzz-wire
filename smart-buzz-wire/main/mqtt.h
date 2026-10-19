#pragma once

#include <stdint.h>

void mqtt_build_topics(void);
void mqtt_start(void);

void mqtt_publish_player(const char *player_uid);
void mqtt_publish_run_start(uint32_t run_id, const char *player_uid);
void mqtt_publish_run_end(uint32_t run_id, const char *player_uid, uint32_t time_ms, uint16_t touches);
void mqtt_publish_run_touch(uint32_t t_ms, uint16_t count);
void mqtt_publish_run_abort(uint32_t run_id);
