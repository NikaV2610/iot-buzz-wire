#pragma once

#include <stdint.h>

void buzzer_init(void);
/* Non-blocking: starts the tone, buzzer_update() switches it off again */
void beep(uint32_t freq_hz, uint32_t duration_ms);
void buzzer_update(void);
