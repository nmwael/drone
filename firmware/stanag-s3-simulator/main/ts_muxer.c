/*
 * TS Muxer - Stub Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "TS_MUXER";

esp_err_t ts_muxer_init(void)
{
    ESP_LOGI(TAG, "TS muxer initialized (stub)");
    return ESP_OK;
}

ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv)
{
    (void)nalu;
    (void)klv;
    ESP_LOGW(TAG, "TS mux called (stub) - returning NULL");
    return NULL;
}