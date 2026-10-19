#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

typedef struct {
    gpio_num_t pin;
    bool stable;        /* true = active (LOW because of pull-up) */
    bool last_raw;
    uint32_t last_change;
    uint16_t hold_ms;
} debounced_t;

/* Returns true if a new "active" edge was just detected */
bool debounce_update(debounced_t *d);
