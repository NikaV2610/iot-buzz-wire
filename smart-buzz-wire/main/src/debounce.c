#include "debounce.h"
#include "config.h"

bool debounce_update(debounced_t *d)
{
    bool raw = (gpio_get_level(d->pin) == 0);
    uint32_t now = millis();

    if (raw != d->last_raw) {
        d->last_raw = raw;
        d->last_change = now;
    }
    if ((now - d->last_change) >= d->hold_ms && raw != d->stable) {
        d->stable = raw;
        return d->stable;
    }
    return false;
}
