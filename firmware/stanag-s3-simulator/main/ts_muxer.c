/*
 * TS Muxer - MPEG-TS Packetization Implementation
 * Creates proper MPEG-TS packets with video (PID 0x100) and KLV (PID 0x101)
 */

#include "stanag_types.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "TS_MUXER";
static const uint16_t VIDEO_PID = 0x100;
static const uint16_t KLV_PID = 0x101;
static uint8_t continuity_counter_video = 0;
static uint8_t continuity_counter_klv = 0;

// Output TS packet as hex dump to serial for capture and analysis
static void ts_packet_hexdump(const uint8_t *data, size_t len, const char *prefix)
{
    char hexbuf[2048];
    size_t max_bytes = (sizeof(hexbuf) - 1) / 3;
    size_t dump_len = (len < max_bytes) ? len : max_bytes;
    
    size_t pos = 0;
    for (size_t i = 0; i < dump_len; i++) {
        pos += snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "%02X ", data[i]);
    }
    if (len > max_bytes) {
        snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "...");
    }
    printf("%s: %s\n", prefix, hexbuf);
}

// Create a single MPEG-TS packet (188 bytes)
static void ts_create_packet(uint8_t *buf, uint16_t pid, uint8_t *cc, 
                             int payload_unit_start, int adaptation_field,
                             const uint8_t *payload, size_t payload_len)
{
    // Clear buffer
    memset(buf, 0xFF, 188);
    
    // TS Header (4 bytes)
    buf[0] = 0x47;  // Sync byte
    buf[1] = (payload_unit_start ? 0x40 : 0x00) | ((pid >> 8) & 0x1F);
    buf[2] = pid & 0xFF;
    buf[3] = 0x10 | (adaptation_field ? 0x20 : 0x00) | ((*cc) & 0x0F);
    (*cc) = ((*cc) + 1) & 0x0F;
    
    size_t offset = 4;
    
    if (adaptation_field) {
        // Adaptation field
        buf[offset++] = 0;  // adaptation_field_length = 0 (no adaptation field data)
    }
    
    // Copy payload
    size_t payload_remaining = 188 - offset;
    size_t copy_len = (payload_len < payload_remaining) ? payload_len : payload_remaining;
    if (payload && copy_len > 0) {
        memcpy(&buf[offset], payload, copy_len);
    }
}

esp_err_t ts_muxer_init(void)
{
    ESP_LOGI(TAG, "MPEG-TS muxer initialized (PID 0x%04X video, PID 0x%04X KLV)", VIDEO_PID, KLV_PID);
    continuity_counter_video = 0;
    continuity_counter_klv = 0;
    return ESP_OK;
}

ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv)
{
    if (!nalu) {
        ESP_LOGW(TAG, "NULL nalu passed to muxer");
        return NULL;
    }
    
    // Allocate TS packet structure (we'll create multiple packets)
    static ts_packet_t ts;
    static uint8_t ts_buffer[188 * 3];  // Up to 3 packets (video + klv + padding)
    
    memset(&ts, 0, sizeof(ts));
    memset(ts_buffer, 0xFF, sizeof(ts_buffer));
    
    size_t packet_count = 0;
    size_t total_len = 0;
    
    // Packet 1: Video NAL unit (PID 0x100)
    uint8_t *video_packet = &ts_buffer[packet_count * 188];
    ts_create_packet(video_packet, VIDEO_PID, &continuity_counter_video, 
                     1, 0, nalu->data, nalu->len);
    packet_count++;
    total_len += 188;
    
    // Packet 2: KLV metadata (PID 0x101) - if available
    if (klv && klv->data && klv->len > 0) {
        uint8_t *klv_packet = &ts_buffer[packet_count * 188];
        ts_create_packet(klv_packet, KLV_PID, &continuity_counter_klv, 
                         1, 0, klv->data, klv->len);
        packet_count++;
        total_len += 188;
        
        ESP_LOGI(TAG, "Muxed KLV: %zu bytes on PID 0x%04X", klv->len, KLV_PID);
    }
    
    // Packet 3: Null packet for timing (if needed)
    if (packet_count < 3) {
        uint8_t *null_packet = &ts_buffer[packet_count * 188];
        ts_create_packet(null_packet, 0x1FFF, &continuity_counter_video, 0, 0, NULL, 0);
        packet_count++;
        total_len += 188;
    }
    
    ts.data = ts_buffer;
    ts.count = packet_count;
    ts.total_len = total_len;
    ts.timestamp_us = nalu->timestamp_us;
    
    // Hex dump for analysis
    ts_packet_hexdump(ts_buffer, total_len, "TS_OUT");
    
    ESP_LOGI(TAG, "Muxed %zu TS packets (%zu bytes total), video_PID=0x%04X, klv_PID=0x%04X", 
             packet_count, total_len, VIDEO_PID, KLV_PID);
    return &ts;
}

void ts_packet_free(ts_packet_t *ts)
{
    // Static buffer, nothing to free
    (void)ts;
}