/*
 * KLV Builder - MISB ST 0601 KLV Metadata Builder Implementation
 */

#include "klv_builder.h"
#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "KLV_BUILDER";

/* KLV encoding: 2-byte key + 2-byte length + value data */
static inline uint8_t* klv_encode_key_length(uint8_t *buf, uint16_t key, uint16_t length) {
    buf[0] = (uint8_t)(key >> 8);
    buf[1] = (uint8_t)(key & 0xFF);
    buf[2] = (uint8_t)(length >> 8);
    buf[3] = (uint8_t)(length & 0xFF);
    return buf + 4;
}

klv_packet_t* klv_builder_build(sensor_data_t *sensors, uint32_t frame_num)
{
    if (!sensors) {
        ESP_LOGE(TAG, "NULL sensor data pointer");
        return NULL;
    }

    klv_packet_t *packet = (klv_packet_t *)malloc(sizeof(klv_packet_t));
    if (!packet) {
        ESP_LOGE(TAG, "Failed to allocate KLV packet struct");
        return NULL;
    }

    /* Calculate total size */
    size_t total_size = 0;

    /* Position: lat(4) + lon(4) + height(4) = 12 bytes */
    total_size += 4 + 12;

    /* Velocity: v_n(4) + v_e(4) + v_d(4) = 12 bytes */
    total_size += 4 + 12;

    /* Heading: heading_deg5 (4 bytes) */
    total_size += 4 + 4;

    /* Baro altitude: altitude in mm as int32 (4 bytes) - cast float to int */
    total_size += 4 + 4;

    /* Temperature: temp * 10 as int16 (2 bytes) - cast float to int */
    total_size += 4 + 2;

    /* Magnetic heading: heading_mag as int16 (4 bytes) - cast float to int */
    total_size += 4 + 4;

    /* Timestamp: tow_ms * 1000 as uint32 (4 bytes) */
    total_size += 4 + 4;

    /* Allocate buffer */
    packet->data = (uint8_t *)malloc(total_size);
    if (!packet->data) {
        ESP_LOGE(TAG, "Failed to allocate KLV buffer");
        free(packet);
        return NULL;
    }

    packet->len = total_size;
    /* Timestamp in us: GPS tow_ms * 1000 */
    packet->timestamp_us = (uint64_t)sensors->gps.tow_ms * 1000;

    uint8_t *ptr = packet->data;

    /* --- Position Key (0x0001) --- */
    ptr = klv_encode_key_length(ptr, ST0601_KEY_POSITION_LLh, 12);
    ptr[0] = (uint8_t)(sensors->gps.lat_deg7 >> 24);
    ptr[1] = (uint8_t)(sensors->gps.lat_deg7 >> 16);
    ptr[2] = (uint8_t)(sensors->gps.lat_deg7 >> 8);
    ptr[3] = (uint8_t)sensors->gps.lat_deg7;
    ptr[4] = (uint8_t)(sensors->gps.lon_deg7 >> 24);
    ptr[5] = (uint8_t)(sensors->gps.lon_deg7 >> 16);
    ptr[6] = (uint8_t)(sensors->gps.lon_deg7 >> 8);
    ptr[7] = (uint8_t)sensors->gps.lon_deg7;
    ptr[8] = (uint8_t)(sensors->gps.height_mm >> 24);
    ptr[9] = (uint8_t)(sensors->gps.height_mm >> 16);
    ptr[10] = (uint8_t)(sensors->gps.height_mm >> 8);
    ptr[11] = (uint8_t)sensors->gps.height_mm;

    /* --- Velocity Key (0x0002) --- */
    ptr = klv_encode_key_length(ptr, ST0601_KEY_VEL_NED, 12);
    ptr[0] = (uint8_t)(sensors->gps.vel_n_mm >> 24);
    ptr[1] = (uint8_t)(sensors->gps.vel_n_mm >> 16);
    ptr[2] = (uint8_t)(sensors->gps.vel_n_mm >> 8);
    ptr[3] = (uint8_t)sensors->gps.vel_n_mm;
    ptr[4] = (uint8_t)(sensors->gps.vel_e_mm >> 24);
    ptr[5] = (uint8_t)(sensors->gps.vel_e_mm >> 16);
    ptr[6] = (uint8_t)(sensors->gps.vel_e_mm >> 8);
    ptr[7] = (uint8_t)sensors->gps.vel_e_mm;
    ptr[8] = (uint8_t)(sensors->gps.vel_d_mm >> 24);
    ptr[9] = (uint8_t)(sensors->gps.vel_d_mm >> 16);
    ptr[10] = (uint8_t)(sensors->gps.vel_d_mm >> 8);
    ptr[11] = (uint8_t)sensors->gps.vel_d_mm;

    /* --- Heading Key (0x0003) --- */
    ptr = klv_encode_key_length(ptr, ST0601_KEY_HEADING, 4);
    ptr[0] = (uint8_t)(sensors->gps.heading_deg5 >> 24);
    ptr[1] = (uint8_t)(sensors->gps.heading_deg5 >> 16);
    ptr[2] = (uint8_t)(sensors->gps.heading_deg5 >> 8);
    ptr[3] = (uint8_t)sensors->gps.heading_deg5;

    /* --- Baro Altitude Key (0x0102) --- */
    /* Cast float altitude to int32 mm */
    int32_t alt_mm = (int32_t)sensors->baro.altitude;
    ptr = klv_encode_key_length(ptr, ST0601_KEY_BARO_ALT, 4);
    ptr[0] = (uint8_t)(alt_mm >> 24);
    ptr[1] = (uint8_t)(alt_mm >> 16);
    ptr[2] = (uint8_t)(alt_mm >> 8);
    ptr[3] = (uint8_t)alt_mm;

    /* --- Temperature Key (0x0103) --- */
    /* Cast float temp to int16 * 10 */
    float temp_val = sensors->aht20.temp;
    int16_t temp_x10 = (int16_t)(temp_val * 10.0f);
    ptr = klv_encode_key_length(ptr, ST0601_KEY_TEMP, 2);
    ptr[0] = (uint8_t)(temp_x10 >> 8);
    ptr[1] = (uint8_t)temp_x10;

    /* --- Magnetic Heading Key (0x0104) --- */
    /* Cast float heading to int32 */
    int32_t mag_heading = (int32_t)sensors->mag.heading;
    ptr = klv_encode_key_length(ptr, ST0601_KEY_MAG_HEADING, 4);
    ptr[0] = (uint8_t)(mag_heading >> 24);
    ptr[1] = (uint8_t)(mag_heading >> 16);
    ptr[2] = (uint8_t)(mag_heading >> 8);
    ptr[3] = (uint8_t)mag_heading;

    /* --- Timestamp Key (private key 0xFFFF) --- */
    ptr = klv_encode_key_length(ptr, 0xFFFF, 4);
    ptr[0] = (uint8_t)(packet->timestamp_us >> 24);
    ptr[1] = (uint8_t)(packet->timestamp_us >> 16);
    ptr[2] = (uint8_t)(packet->timestamp_us >> 8);
    ptr[3] = (uint8_t)packet->timestamp_us;

    ESP_LOGD(TAG, "Built KLV packet: %u bytes for frame %u", packet->len, frame_num);
    return packet;
}

void klv_packet_free(klv_packet_t *packet)
{
    if (packet) {
        if (packet->data) {
            free(packet->data);
        }
        free(packet);
    }
}

esp_err_t klv_builder_init(void)
{
    ESP_LOGI(TAG, "KLV builder initialized (MISB ST 0601)");
    return ESP_OK;
}