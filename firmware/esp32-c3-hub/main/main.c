/*
 * ESP32-C3 Sensor Hub Main Entry
 * GPS + IMU + Mag + Baro + Temp/Humidity + EKF Fusion
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_now.h"

#include "gps_gnss.h"
#include "magnetometer.h"
#include "imu_baro.h"
#include "aht20.h"
#include "bmp280.h"
#include "sensor_fusion_ekf.h"
#include "comms_espnow.h"
#include "comms_uart_rp2040.h"
#include "telemetry_provider.h"

static const char *TAG = "HUB_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-C3 Sensor Hub Firmware");
    ESP_LOGI(TAG, "Free heap: %zu bytes", esp_get_free_heap_size());

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

    // Initialize WiFi for ESP-NOW
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());

    // Initialize I2C bus for sensors
    ESP_LOGI(TAG, "Initializing I2C sensors...");
    ESP_ERROR_CHECK(gps_gnss_init());
    ESP_ERROR_CHECK(magnetometer_init());
    ESP_ERROR_CHECK(imu_baro_init());
    ESP_ERROR_CHECK(aht20_init());
    ESP_ERROR_CHECK(bmp280_init());

    // Initialize sensor fusion EKF
    ESP_LOGI(TAG, "Initializing EKF fusion...");
    ESP_ERROR_CHECK(sensor_fusion_ekf_init());

    // Initialize telemetry provider for S3
    ESP_LOGI(TAG, "Initializing telemetry provider...");
    ESP_ERROR_CHECK(telemetry_provider_init());

    // Initialize ESP-NOW comms to S3
    ESP_LOGI(TAG, "Initializing ESP-NOW to S3...");
    ESP_ERROR_CHECK(comms_espnow_init());

    // Initialize UART comms to RP2040s
    ESP_LOGI(TAG, "Initializing UART to RP2040s...");
    ESP_ERROR_CHECK(comms_uart_rp2040_init());

    // Start sensor tasks
    ESP_LOGI(TAG, "Starting sensor tasks...");
    gps_gnss_start();
    magnetometer_start();
    imu_baro_start();
    aht20_start();
    bmp280_start();
    sensor_fusion_ekf_start();
    telemetry_provider_start();

    ESP_LOGI(TAG, "Sensor hub running!");

    // Main loop - monitor health
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Heap: %zu | Tasks: %u | EKF: %s",
                 esp_get_free_heap_size(),
                 uxTaskGetNumberOfTasks(),
                 sensor_fusion_ekf_is_converged() ? "CONVERGED" : "CONVERGING");
    }
}