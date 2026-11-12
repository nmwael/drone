/*
 * Network TX - ESP-NOW/UDP Implementation
 */

#include "stanag_types.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "string.h"

static const char *TAG = "NETWORK_TX";

// Default broadcast MAC for ESP-NOW
static uint8_t s_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_espnow_peer_added = false;

esp_err_t network_tx_init(void)
{
    ESP_LOGI(TAG, "Network TX initialized (ESP-NOW/UDP)");
    
    // In QEMU, WiFi is not started, so ESP-NOW peer addition will crash
    // Skip ESP-NOW peer addition in QEMU mode
    // For hardware testing, uncomment the ESP-NOW peer addition code below
    ESP_LOGW(TAG, "ESP-NOW peer addition skipped (QEMU mode)");
    ESP_LOGI(TAG, "For hardware testing, enable ESP-NOW peer addition in network_tx.c");
    
    /*
    // Add broadcast peer for ESP-NOW
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, s_broadcast_mac, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;
    
    esp_err_t ret = esp_now_add_peer(&peer_info);
    if (ret == ESP_OK || ret == ESP_ERR_ESPNOW_EXIST) {
        s_espnow_peer_added = true;
        ESP_LOGI(TAG, "ESP-NOW broadcast peer added");
    } else {
        ESP_LOGW(TAG, "Failed to add ESP-NOW peer: %s", esp_err_to_name(ret));
    }
    */
    
    return ESP_OK;
}

void network_tx_send(ts_packet_t *ts)
{
    if (!ts || !ts->data || ts->total_len == 0) {
        ESP_LOGW(TAG, "Invalid TS packet");
        return;
    }
    
    // In QEMU/simulation, ESP-NOW send will fail
    // Log the attempt for debugging
    if (s_espnow_peer_added) {
        esp_err_t ret = esp_now_send(s_broadcast_mac, ts->data, ts->total_len);
        if (ret != ESP_OK) {
            ESP_LOGD(TAG, "ESP-NOW send failed: %s (expected in QEMU)", esp_err_to_name(ret));
        } else {
            ESP_LOGD(TAG, "ESP-NOW sent %zu bytes", ts->total_len);
        }
    } else {
        ESP_LOGD(TAG, "ESP-NOW peer not added, skipping send (%zu bytes)", ts->total_len);
    }
}

void network_tx_deinit(void)
{
    if (s_espnow_peer_added) {
        esp_now_del_peer(s_broadcast_mac);
        s_espnow_peer_added = false;
        ESP_LOGI(TAG, "ESP-NOW peer removed");
    }
}