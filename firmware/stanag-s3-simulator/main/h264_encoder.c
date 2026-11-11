/*
 * H.264 Encoder - Stub Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "H264_ENC";

esp_err_t h264_encoder_init(void)
{
    ESP_LOGI(TAG, "H.264 encoder initialized (stub)");
    return ESP_OK;
}

h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv)
{
    (void)yuv;
    ESP_LOGW(TAG, "H.264 encode called (stub) - returning NULL");
    return NULL;
}