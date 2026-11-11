/*
 * UART Telemetry - Stub Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "UART_TELEM";

esp_err_t uart_telemetry_init(void)
{
    ESP_LOGI(TAG, "UART telemetry initialized (stub)");
    return ESP_OK;
}

void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num)
{
    (void)sensors;
    (void)nalu;
    (void)frame_num;
    // No-op
}