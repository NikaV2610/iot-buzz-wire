#pragma once

#include <stdbool.h>
#include <stdint.h>

void strip_init(void);
/* Shows the best-run "ghost" LED; elapsed_ms is the current run time */
void best_run_led_update(bool running, uint32_t elapsed_ms, uint32_t best_run_time_ms);
