/*
 * Network TX - Stub Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "NETWORK_TX";

esp_err_t network_tx_init(void)
{
    ESP_LOGI(TAG, "Network TX initialized (stub)");
    return ESP_OK;
}

void network_tx_send(ts_packet_t *ts)
{
    (void)ts;
    // No-op
}