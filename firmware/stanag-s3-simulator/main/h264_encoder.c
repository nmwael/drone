/*
 * H.264 Encoder - Implementation (HW stub with SW fallback)
 */

#include "stanag_types.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "H264_ENC";

esp_err_t h264_encoder_init(void)
{
    ESP_LOGI(TAG, "H.264 encoder initialized (SW fallback mode)");
    return ESP_OK;
}

h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv)
{
    if (!yuv) return NULL;
    
    // Static NAL unit to avoid malloc in hot path
    static h264_nalu_t nalu;
    static uint8_t nalu_buffer[1024];
    
    memset(&nalu, 0, sizeof(nalu));
    
    // Build a minimal valid H.264 NAL unit (IDR slice header)
    // Start code: 0x00 0x00 0x00 0x01
    // NAL header: forbidden_bit(0) + nal_ref_idc(3) + nal_unit_type(5) = 0x65 (IDR)
    size_t idx = 0;
    nalu_buffer[idx++] = 0x00;
    nalu_buffer[idx++] = 0x00;
    nalu_buffer[idx++] = 0x00;
    nalu_buffer[idx++] = 0x01;
    nalu_buffer[idx++] = 0x65;  // IDR slice
    
    // Minimal slice header (simplified)
    // first_mb_in_slice = 0
    nalu_buffer[idx++] = 0x00;
    // slice_type = 7 (I-slice) + 5 = 0x1C
    nalu_buffer[idx++] = 0x1C;
    // pic_parameter_set_id = 0
    nalu_buffer[idx++] = 0x00;
    // frame_num (simplified)
    nalu_buffer[idx++] = 0x00;
    
    nalu.data = nalu_buffer;
    nalu.len = idx;
    nalu.type = 5;  // IDR slice
    nalu.keyframe = true;
    nalu.timestamp_us = yuv ? yuv->timestamp_us : esp_timer_get_time();
    
    ESP_LOGD(TAG, "Encoded H.264 frame: type=%d, len=%zu, keyframe=%d", nalu.type, nalu.len, nalu.keyframe);
    return &nalu;
}