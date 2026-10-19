#pragma once

#include <stdbool.h>

void rfid_init(void);
/* Non-blocking: copies a newly scanned UID into uid (RFID_UID_LEN bytes), returns true if one was available */
bool rfid_get_uid(char *uid);
