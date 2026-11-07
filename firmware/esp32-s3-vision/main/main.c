/*
 * ESP32-S3 Vision Main Entry
 * STANAG 4609 H.264 + Thermal/IR Vision + TFLite Inference
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiram.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "thermal_camera.h"
#include "ir_camera.h"
#include "h264_encoder.h"
#include "klv_builder.h"
#include "ts_muxer.h"
#include "wifi_stream.h"
#include "comms_uart.h"
#include "vision_inference.h"

static const char *TAG = "VISION_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-S3 Vision Firmware");
    ESP_LOGI(TAG, "Free heap: %zu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "PSRAM size: %zu bytes", esp_spiram_get_size());

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize PSRAM check
    if (!esp_spiram_is_initialized()) {
        ESP_LOGE(TAG, "PSRAM not initialized! H.264 encoding will fail.");
        return;
    }

    // Initialize subsystems
    ESP_LOGI(TAG, "Initializing cameras...");
    ESP_ERROR_CHECK(thermal_camera_init());
    ESP_ERROR_CHECK(ir_camera_init());

    ESP_LOGI(TAG, "Initializing H.264 encoder...");
    ESP_ERROR_CHECK(h264_encoder_init());

    ESP_LOGI(TAG, "Initializing KLV builder...");
    ESP_ERROR_CHECK(klv_builder_init());

    ESP_LOGI(TAG, "Initializing TS muxer...");
    ESP_ERROR_CHECK(ts_muxer_init());

    ESP_LOGI(TAG, "Initializing WiFi/ESP-NOW stream...");
    ESP_ERROR_CHECK(wifi_stream_init());

    ESP_LOGI(TAG, "Initializing UART comms to C3...");
    ESP_ERROR_CHECK(comms_uart_init());

    ESP_LOGI(TAG, "Initializing vision inference...");
    ESP_ERROR_CHECK(vision_inference_init());

    // Start processing tasks
    ESP_LOGI(TAG, "Starting capture/encode/stream tasks...");
    thermal_camera_start();
    ir_camera_start();
    h264_encoder_start();
    wifi_stream_start();
    vision_inference_start();

    ESP_LOGI(TAG, "Vision system running!");

    // Main loop - monitor health
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Heap: %zu | PSRAM: %zu | Tasks: %u",
                 esp_get_free_heap_size(),
                 esp_spiram_get_free_size(),
                 uxTaskGetNumberOfTasks());
    }
}