/*
 * UART Telemetry - JSON Output Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "UART_TELEM";

esp_err_t uart_telemetry_init(void)
{
    ESP_LOGI(TAG, "UART telemetry initialized (JSON output)");
    return ESP_OK;
}

void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num)
{
    if (!sensors || !nalu) {
        ESP_LOGW(TAG, "Invalid telemetry parameters");
        return;
    }
    
    // Build JSON telemetry string
    char json_buffer[512];
    int len = snprintf(json_buffer, sizeof(json_buffer),
        "{"
        "\"frame\":%lu,"
        "\"timestamp_us\":%llu,"
        "\"gps\":{"
        "\"lat\":%.6f,"
        "\"lon\":%.6f,"
        "\"alt\":%.1f,"
        "\"heading\":%.1f,"
        "\"fix\":%d,"
        "\"sats\":%d"
        "},"
        "\"imu\":{"
        "\"roll\":%.1f,"
        "\"pitch\":%.1f,"
        "\"yaw\":%.1f"
        "},"
        "\"mag\":{"
        "\"heading\":%.1f"
        "},"
        "\"baro\":{"
        "\"altitude\":%.1f,"
        "\"pressure\":%.1f,"
        "\"temp\":%.1f"
        "},"
        "\"aht20\":{"
        "\"temp\":%.1f,"
        "\"humidity\":%.1f"
        "},"
        "\"h264\":{"
        "\"type\":%d,"
        "\"keyframe\":%s,"
        "\"len\":%zu"
        "}"
        "}",
        frame_num,
        nalu->timestamp_us,
        sensors->gps.lat, sensors->gps.lon, sensors->gps.alt, sensors->gps.heading,
        sensors->gps.fix, sensors->gps.sats,
        sensors->imu.roll, sensors->imu.pitch, sensors->imu.yaw,
        sensors->mag.heading,
        sensors->baro.altitude, sensors->baro.pressure, sensors->baro.temp,
        sensors->aht20.temp, sensors->aht20.humidity,
        nalu->type, nalu->keyframe ? "true" : "false", nalu->len
    );
    
    if (len > 0 && len < (int)sizeof(json_buffer)) {
        // Output to UART (Serial)
        printf("%s\n", json_buffer);
        ESP_LOGD(TAG, "Telemetry sent: frame=%lu, len=%d", frame_num, len);
    } else {
        ESP_LOGW(TAG, "Telemetry buffer overflow");
    }
}