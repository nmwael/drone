/*
 * STANAG S3 Simulator - Main Entry Point
 * JPEG loop + Simulated sensors + H.264 + KLV + MPEG-TS + ESP-NOW/UDP
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_psram.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_task_wdt.h"

#include "jpeg_loader.h"
#include "sensor_sim.h"
#include "h264_encoder.h"
#include "klv_builder.h"
#include "ts_muxer.h"
#include "network_tx.h"
#include "uart_telemetry.h"

static const char *TAG = "STANAG_POC";

#define FRAME_RATE_HZ 30
#define FRAME_INTERVAL_MS (1000 / FRAME_RATE_HZ)

void app_main(void)
{
    // Disable task watchdog for QEMU testing
    esp_task_wdt_deinit();
    ESP_LOGI(TAG, "Task watchdog disabled");
    
    // Early printf to ensure output before any crash
    printf("\n\n=== STANAG S3 Simulator Starting ===\n");
    fflush(stdout);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "STANAG S3 Simulator Starting");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "PSRAM: %s", esp_psram_is_initialized() ? "OK" : "NOT INITIALIZED");

    // Initialize NVS (non-fatal; may fail on first boot in QEMU where
    // the flash image has no pre-formatted NVS partition)
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS init failed (%s), erasing...", esp_err_to_name(ret));
        esp_err_t ret2 = nvs_flash_erase();
        if (ret2 == ESP_OK) {
            ret = nvs_flash_init();
        }
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS unavailable, continuing without it (%s)", esp_err_to_name(ret));
    }

    // Initialize network stack (needed for ESP-NOW/UDP)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // NOTE: WiFi/ESP-NOW initialization is SKIPPED in QEMU
    // WiFi phy calibration blocks the main task indefinitely in QEMU
    // For hardware testing, uncomment the WiFi/ESP-NOW section below
    ESP_LOGW(TAG, "WiFi/ESP-NOW initialization skipped (QEMU mode)");
    ESP_LOGI(TAG, "For hardware testing, enable WiFi section in main.c and call network_tx_espnow_init()");
    
    /*
    // Initialize WiFi for ESP-NOW/UDP
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    esp_err_t wifi_ret = esp_wifi_start();
    if (wifi_ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi start failed (code: %s)", esp_err_to_name(wifi_ret));
    } else {
        ESP_LOGI(TAG, "WiFi started successfully");
        
        // Initialize ESP-NOW after WiFi start
        ESP_ERROR_CHECK(network_tx_espnow_init());
    }
    */

    // Initialize subsystems
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    ESP_ERROR_CHECK(jpeg_loader_init());
    ESP_LOGI(TAG, "SPIFFS initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing sensor simulators...");
    ESP_ERROR_CHECK(sensor_sim_init());
    ESP_LOGI(TAG, "Sensor simulators initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing H.264 encoder...");
    ESP_ERROR_CHECK(h264_encoder_init());
    ESP_LOGI(TAG, "H.264 encoder initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing KLV builder...");
    ESP_ERROR_CHECK(klv_builder_init());
    ESP_LOGI(TAG, "KLV builder initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing TS muxer...");
    ESP_ERROR_CHECK(ts_muxer_init());
    ESP_LOGI(TAG, "TS muxer initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing network TX (ESP-NOW/UDP)...");
    /* Uncomment for QEMU/devcontainer testing with Unix socket.
       This bypasses WSL2 UDP loopback limitations.
       Requires MediaMTX configured with source: unix+mpegts:///tmp/stanag.sock
       Real hardware (UDP) is unaffected when this is commented out. */
    network_tx_set_unix_socket("/tmp/stanag.sock");
    ESP_ERROR_CHECK(network_tx_init());
    ESP_LOGI(TAG, "Network TX initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing UART telemetry...");
    ESP_ERROR_CHECK(uart_telemetry_init());
    ESP_LOGI(TAG, "UART telemetry initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    // Start tasks
    ESP_LOGI(TAG, "Starting main loop @ %d fps", FRAME_RATE_HZ);

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t frame_ticks = pdMS_TO_TICKS(1000 / FRAME_RATE_HZ);
    uint32_t frame_count = 0;

    ESP_LOGI(TAG, "Entering main loop...");
    printf(">>> Entering main loop...\n");
    fflush(stdout);
    
    while (1) {
        printf(">>> Main loop iteration %lu\n", frame_count);
        fflush(stdout);
        ESP_LOGI(TAG, "Main loop iteration %lu", frame_count);
        
        // Frame timing
        vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
        taskYIELD();  // Ensure other tasks get CPU time in QEMU

        // 1. Get next JPEG frame (loops 0-4)
        jpeg_frame_t *frame = jpeg_loader_get_next();
        if (!frame) {
            ESP_LOGE(TAG, "Failed to get JPEG frame");
            continue;
        }

        // 2. Decode JPEG to YUV420 (HW JPEG decoder)
        yuv420_frame_t *yuv = jpeg_loader_decode(frame);
        if (!yuv) {
            ESP_LOGE(TAG, "JPEG decode failed");
            continue;
        }

        // 3. Encode YUV420 to H.264 (HW encoder)
        h264_nalu_t *nalu = h264_encoder_encode(yuv);
        if (!nalu) {
            ESP_LOGE(TAG, "H.264 encode failed");
            jpeg_loader_free_yuv(yuv);
            continue;
        }

        // 4. Get simulated sensor data
        sensor_data_t sensors;
        sensor_sim_get_data(&sensors, frame_count);

        // 5. Build KLV packet from sensor data
        klv_packet_t *klv = klv_builder_build(&sensors, frame_count);
        if (!klv) {
            ESP_LOGE(TAG, "KLV build failed");
        }

        // 6. Mux H.264 + KLV into MPEG-TS
        ts_packet_t *ts = ts_muxer_mux(nalu, klv);
        if (!ts) {
            ESP_LOGE(TAG, "TS mux failed");
        }

        // 7. Transmit via ESP-NOW/UDP
        if (ts) {
            network_tx_send(ts);
        }

        // 8. Send telemetry via UART (JSON)
        uart_telemetry_send(&sensors, nalu, frame_count);

        // Cleanup
        if (klv) klv_packet_free(klv);
        // ts_packet_free(ts);  // Static buffer, no free needed
        jpeg_loader_free_yuv(yuv);

        // Stats
        frame_count++;
        if (frame_count % 30 == 0) {
            ESP_LOGI(TAG, "Frame %lu | GPS: %.6f, %.6f | Alt: %.1fm | Hdg: %.1f° | Temp: %.1f°C",
                      frame_count,
                      sensors.gps.lat, sensors.gps.lon,
                      sensors.gps.alt, sensors.mag.heading,
                      sensors.aht20.temp);
        }

        // Frame budget check
        uint32_t elapsed = (xTaskGetTickCount() - last_wake) * portTICK_PERIOD_MS;
        if (elapsed > FRAME_INTERVAL_MS) {
            ESP_LOGW(TAG, "Frame overrun: %lu ms (budget %d ms)", elapsed, FRAME_INTERVAL_MS);
        }
    }
}