#pragma once

#include <stdint.h>

void game_init(void);
/* Call periodically from the main loop */
void game_update(void);

/* Thread-safe setters, called from the MQTT task */
void game_set_best_run_time(uint32_t time_ms);   /* 0 = no best run yet */
void game_request_reset(void);

/* For the output modules */
int game_is_running(void);
uint32_t game_elapsed_ms(void);
uint32_t game_best_run_time(void);
