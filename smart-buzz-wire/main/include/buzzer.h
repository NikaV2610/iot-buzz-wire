#pragma once

#include <stdint.h>

/* One sound per game action */
typedef enum {
    SOUND_CARD,        /* card read:      two short beeps */
    SOUND_START,       /* run started:    rising "go" melody */
    SOUND_COLLISION,   /* wire touched:   low buzz */
    SOUND_STOP,        /* run finished:   finish fanfare */
    SOUND_BUTTON,      /* cal button:     one short click */
} sound_t;

void buzzer_init(void);
/* Non-blocking: starts the sound, buzzer_update() plays the next notes */
void buzzer_play(sound_t sound);
void buzzer_update(void);
