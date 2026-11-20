/*
 * STANAG S3 Simulator - Main Entry Point
 * JPEG loop + Simulated sensors + H.264 + KLV + MPEG-TS + ESP-NOW/UDP
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
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
#include "srt.h"
#include "uart_telemetry.h"
#include "srt_endpoint.h"
#include "status_led.h"

static const char *TAG = "STANAG_POC";

#define FRAME_RATE_HZ 30
#define FRAME_INTERVAL_MS (1000 / FRAME_RATE_HZ)

/* Global link-state flag (set from the IP_EVENT handler) */
static volatile bool s_wifi_got_ip = false;

/* WiFi event handler */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to AP");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "Disconnected from AP, reason: %d, retrying...", disconn->reason);
                esp_wifi_connect();
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
                ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
                ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
                s_wifi_got_ip = true;
                break;
            case IP_EVENT_STA_LOST_IP:
                s_wifi_got_ip = false;
                break;
            default:
                break;
        }
    }
}

void app_main(void)
{
    // Disable task watchdog for QEMU testing
    esp_task_wdt_deinit();
    ESP_LOGI(TAG, "Task watchdog disabled");

    // Status LED (boot pattern until WiFi/SRT state is known)
    status_led_init();
    status_led_set(LED_STATE_BOOT);
    status_led_update();  /* Force immediate render of boot pattern */

#if CONFIG_STATUS_LED_SCAN
    /* Diagnostic: identify which GPIO the onboard LED is on */
    status_led_pin_scan();
#endif
    
    // Early printf to ensure output before any crash
    printf("\n\n=== STANAG S3 Simulator Starting ===\n");
    fflush(stdout);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "STANAG S3 Simulator Starting");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "PSRAM: NOT AVAILABLE (running on internal RAM)");

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

    // Create default WiFi STA interface
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize WiFi for ESP-NOW/UDP
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    
    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    // Configure WiFi credentials (set via menuconfig or runtime)
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    // If SSID is empty, skip connection (QEMU/testing mode)
    if (strlen((char*)wifi_config.sta.ssid) > 0) {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_LOGI(TAG, "WiFi configured for SSID: %s", wifi_config.sta.ssid);
    } else {
        ESP_LOGW(TAG, "No WiFi SSID configured, running in ESP-NOW only mode");
    }
    
    esp_err_t wifi_ret = esp_wifi_start();
    if (wifi_ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi start failed (code: %s)", esp_err_to_name(wifi_ret));
        ESP_LOGW(TAG, "Continuing without WiFi (QEMU mode)");
    } else {
        ESP_LOGI(TAG, "WiFi started successfully");
        
        // Initialize ESP-NOW after WiFi start (works with or without AP connection)
        ESP_ERROR_CHECK(network_tx_espnow_init());
        
        // Configure UDP destination for MediaMTX
        network_tx_set_udp_dest("192.168.4.2", 8888);
        
        // Connect to WiFi if credentials provided
        if (strlen((char*)wifi_config.sta.ssid) > 0) {
            ESP_LOGI(TAG, "Connecting to WiFi...");
            esp_wifi_connect();
        }
    }

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

    ESP_LOGI(TAG, "Initializing network TX (ESP-NOW/UDP/SRT)...");
    /* Uncomment for QEMU/devcontainer testing with Unix socket.
       This bypasses WSL2 UDP loopback limitations.
       Requires MediaMTX configured with source: unix+mpegts:///tmp/stanag.sock
       Real hardware (UDP) is unaffected when this is commented out. */
    // network_tx_set_unix_socket("/tmp/stanag.sock");

    /* SRT Configuration — endpoint + credentials come from srt_endpoint.h
       (gitignored; see srt_endpoint.h.example). Do NOT commit secrets. */
    network_tx_set_mode(NETWORK_TX_MODE_RTMP);
    /* RTMP Configuration — use RTMP instead of SRT since SRT is blocked
       by server-side firewall on 172.160.27.93:8890 */
    network_tx_set_rtmp_config(
        "172.160.27.93", 1935,          /* server:port */
        "live",                          /* app */
        "sse_drone_f71919",              /* stream */
        "sse_drone_f71919",              /* user */
        "5eefeb2389cf8b2ac83218ae926e7143"  /* pass */
    );

    /* UDP Configuration (commented out - works with MediaMTX)
       network_tx_set_mode(NETWORK_TX_MODE_UDP);
       network_tx_set_udp_dest("192.168.4.2", 8888);  // MediaMTX IP:port
    */

    ESP_ERROR_CHECK(network_tx_init());

    /* Initialize RTMP if using RTMP mode (after network_tx_init).
       Non-fatal: retried in the main loop until the network is reachable. */
    if (network_tx_get_mode() == NETWORK_TX_MODE_RTMP) {
        esp_err_t rtmp_ret = network_tx_rtmp_init();
        if (rtmp_ret != ESP_OK) {
            ESP_LOGW(TAG, "RTMP init deferred (will retry): %s", esp_err_to_name(rtmp_ret));
        }
    }
    
    ESP_LOGI(TAG, "Network TX initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Initializing UART telemetry...");
    ESP_ERROR_CHECK(uart_telemetry_init());
    ESP_LOGI(TAG, "UART telemetry initialized OK");
    vTaskDelay(pdMS_TO_TICKS(10));

    // Start tasks
    ESP_LOGI(TAG, "Starting main loop @ %d fps", FRAME_RATE_HZ);

    TickType_t last_wake = xTaskGetTickCount();
    uint32_t frame_count = 0;
    TickType_t last_srt_retry = last_wake;
    const TickType_t srt_retry_ticks = pdMS_TO_TICKS(2000);  /* retry every 2s */

    ESP_LOGI(TAG, "Entering main loop...");
    printf(">>> Entering main loop...\n");
    fflush(stdout);
    
    while (1) {
        printf(">>> Main loop iteration %lu\n", frame_count);
        fflush(stdout);
        ESP_LOGI(TAG, "Main loop iteration %lu", frame_count);

        /* ---- Status LED + SRT retry (runs every iteration, cheap) ---- */
        TickType_t now = xTaskGetTickCount();
        status_led_update();

        /* LED state: RTMP streaming > WiFi up > WiFi waiting */
        if (network_tx_get_mode() == NETWORK_TX_MODE_RTMP) {
            if (network_tx_rtmp_is_connected()) {
                status_led_set(LED_STATE_RTMP_ON);
            } else if (s_wifi_got_ip) {
                status_led_set(LED_STATE_RTMP_WAIT);
                /* Time-based reconnect attempt */
                if (now - last_srt_retry >= srt_retry_ticks) {
                    last_srt_retry = now;
                    ESP_LOGI(TAG, "Retrying RTMP connection...");
                    network_tx_rtmp_deinit();
                    network_tx_rtmp_init();
                }
            } else {
                status_led_set(LED_STATE_WIFI_WAIT);
            }
        }

        // Frame timing
        vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
        taskYIELD();  // Ensure other tasks get CPU time in QEMU

        // 1. Get simulated sensor data first
        sensor_data_t sensors;
        sensor_sim_get_data(&sensors, frame_count);

        /* Print JSON telemetry to stdout for JSON ingestion pipeline */
        printf("{\"frame\":%" PRIu32 ",\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"heading\":%.1f,\"ts\":%" PRIu32 "}\n",
               frame_count,
               sensors.gps.lat, sensors.gps.lon, sensors.gps.alt, sensors.gps.heading,
               (uint32_t)(sensors.gps.tow_ms * 1000));
        fflush(stdout);

        // 2. Get next JPEG frame (loops 0-4)
        jpeg_frame_t *frame = jpeg_loader_get_next();
        if (!frame) {
            ESP_LOGE(TAG, "Failed to get JPEG frame");
            continue;
        }

        // 3. Decode JPEG to YUV420 (HW JPEG decoder)
        yuv420_frame_t *yuv = jpeg_loader_decode(frame);
        if (!yuv) {
            ESP_LOGE(TAG, "JPEG decode failed");
            continue;
        }

        // 4. Encode YUV420 to H.264 (HW encoder)
        h264_nalu_t *nalu = h264_encoder_encode(yuv);
        if (!nalu) {
            ESP_LOGE(TAG, "H.264 encode failed");
            jpeg_loader_free_yuv(yuv);
            continue;
        }

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

        // 7. Transmit via ESP-NOW/UDP/SRT
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