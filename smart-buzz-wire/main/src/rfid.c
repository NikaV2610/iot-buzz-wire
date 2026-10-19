#include "rfid.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

/*
  RFID (RC522, component abobija/rc522)
  Note: The API of this component differs between versions.
  This code follows the 3.x variant; if it differs, check the component's README.
*/
static QueueHandle_t uid_queue = NULL;

static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        char raw[32];
        rc522_picc_uid_to_str(&picc->uid, raw, sizeof(raw));

        /* Remove spaces -> e.g. "04a1b2c3" */
        char uid[RFID_UID_LEN];
        int j = 0;
        for (int i = 0; raw[i] && j < (int)sizeof(uid) - 1; i++) {
            if (raw[i] != ' ') uid[j++] = raw[i];
        }
        uid[j] = '\0';

        xQueueSend(uid_queue, uid, 0);   /* processed in the game loop */
    }
}

void rfid_init(void)
{
    uid_queue = xQueueCreate(4, sizeof(char[RFID_UID_LEN]));

    rc522_spi_config_t driver_cfg = {
        .host_id = SPI3_HOST,
        .bus_config = &(spi_bus_config_t){
            .miso_io_num = PIN_RFID_MISO,
            .mosi_io_num = PIN_RFID_MOSI,
            .sclk_io_num = PIN_RFID_SCK,
            .quadwp_io_num = -1,   /* unused: otherwise 0 would mean GPIO0 */
            .quadhd_io_num = -1,
        },
        .dev_config = {
            .spics_io_num = PIN_RFID_SS,
        },
        .rst_io_num = PIN_RFID_RST,
    };

    rc522_driver_handle_t driver;
    ESP_ERROR_CHECK(rc522_spi_create(&driver_cfg, &driver));
    ESP_ERROR_CHECK(rc522_driver_install(driver));

    rc522_config_t scanner_cfg = { .driver = driver };
    rc522_handle_t scanner;
    ESP_ERROR_CHECK(rc522_create(&scanner_cfg, &scanner));
    ESP_ERROR_CHECK(rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED,
                                          on_picc_state_changed, NULL));
    ESP_ERROR_CHECK(rc522_start(scanner));
}

bool rfid_get_uid(char *uid)
{
    return xQueueReceive(uid_queue, uid, 0) == pdTRUE;
}
