/*
 * UART Telemetry - JSON Output via UART
 *
 * Sends JSON telemetry to ESP32-C3 via UART
 */

#include "stanag_types.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "UART_TELEM";

/* UART configuration */
#define UART_TX_GPIO      17  /* TX pin */
#define UART_RX_GPIO      18  /* RX pin */
#define UART_BAUD_RATE    115200
#define UART_PORT         UART_NUM_1
#define UART_BUF_SIZE     2048

static bool s_uart_initialized = false;

esp_err_t uart_telemetry_init(void)
{
    ESP_LOGI(TAG, "Initializing UART telemetry on UART%d (TX=%d, RX=%d, %d baud)",
             UART_PORT, UART_TX_GPIO, UART_RX_GPIO, UART_BAUD_RATE);

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(UART_PORT, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        uart_driver_delete(UART_PORT);
        return ret;
    }

    ret = uart_set_pin(UART_PORT, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        uart_driver_delete(UART_PORT);
        return ret;
    }

    s_uart_initialized = true;
    ESP_LOGI(TAG, "UART telemetry initialized");
    return ESP_OK;
}

/* Forward declaration for fallback function */
static void uart_telemetry_send_fallback(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num);

void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num)
{
    if (!sensors || !nalu) {
        ESP_LOGW(TAG, "Invalid telemetry parameters");
        return;
    }

    if (!s_uart_initialized) {
        /* Fallback to stdout for debugging */
        uart_telemetry_send_fallback(sensors, nalu, frame_num);
        return;
    }

    /* Build JSON telemetry string */
    char json_buffer[1024];
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
        "}\n",
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
        /* Send via UART */
        int written = uart_write_bytes(UART_PORT, json_buffer, len);
        if (written != len) {
            ESP_LOGW(TAG, "UART partial write: %d/%d bytes", written, len);
        }
        ESP_LOGD(TAG, "Telemetry sent: frame=%lu, len=%d", frame_num, len);
    } else {
        ESP_LOGW(TAG, "Telemetry buffer overflow or error");
    }
}

/* Fallback to stdout (for QEMU/testing) */
void uart_telemetry_send_fallback(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num)
{
    char json_buffer[1024];
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
        "}\n",
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
        printf("%s", json_buffer);
        ESP_LOGD(TAG, "Telemetry (fallback): frame=%lu, len=%d", frame_num, len);
    }
}

void uart_telemetry_deinit(void)
{
    if (s_uart_initialized) {
        uart_driver_delete(UART_PORT);
        s_uart_initialized = false;
        ESP_LOGI(TAG, "UART telemetry deinitialized");
    }
}