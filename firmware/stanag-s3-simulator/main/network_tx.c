/*
 * Network TX - ESP-NOW/UDP Implementation
 *
 * Supports both ESP-NOW (for direct drone-to-ground) and UDP (for WiFi/network)
 */

#include "stanag_types.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* MAC address formatting macros (from esp_rom/ets_sys.h) */
#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

static const char *TAG = "NETWORK_TX";

/* Configuration - set via menuconfig or runtime */
static struct {
    bool use_espnow;
    bool use_udp;
    uint8_t espnow_peer_mac[6];
    char udp_dst_ip[16];
    uint16_t udp_dst_port;
    int udp_socket;
    struct sockaddr_in udp_dest_addr;
    bool initialized;
} s_net_config = {
    .use_espnow = true,
    .use_udp = true,
    .espnow_peer_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* Broadcast */
    .udp_dst_ip = "192.168.4.2",  /* Default MediaMTX IP */
    .udp_dst_port = 8888,
    .udp_socket = -1,
    .initialized = false
};

static bool s_espnow_peer_added = false;

/* Forward declaration for ESP-NOW send callback */
static void network_tx_espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);

esp_err_t network_tx_init(void)
{
    ESP_LOGI(TAG, "Network TX initializing (ESP-NOW: %s, UDP: %s)",
             s_net_config.use_espnow ? "yes" : "no",
             s_net_config.use_udp ? "yes" : "no");

    esp_err_t ret = ESP_OK;

    /* Initialize UDP socket if enabled */
    if (s_net_config.use_udp) {
        s_net_config.udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (s_net_config.udp_socket < 0) {
            ESP_LOGE(TAG, "Failed to create UDP socket: errno %d", errno);
            ret = ESP_FAIL;
        } else {
            /* Set non-blocking */
            int flags = fcntl(s_net_config.udp_socket, F_GETFL, 0);
            fcntl(s_net_config.udp_socket, F_SETFL, flags | O_NONBLOCK);

            memset(&s_net_config.udp_dest_addr, 0, sizeof(s_net_config.udp_dest_addr));
            s_net_config.udp_dest_addr.sin_family = AF_INET;
            s_net_config.udp_dest_addr.sin_port = htons(s_net_config.udp_dst_port);
            inet_pton(AF_INET, s_net_config.udp_dst_ip, &s_net_config.udp_dest_addr.sin_addr);

            ESP_LOGI(TAG, "UDP socket created, target: %s:%d", s_net_config.udp_dst_ip, s_net_config.udp_dst_port);
        }
    }

    /* Initialize ESP-NOW if enabled */
    if (s_net_config.use_espnow) {
        /* ESP-NOW requires WiFi to be started */
        /* In QEMU/simulation mode, WiFi is not started, so we skip */
        /* For hardware, WiFi must be initialized before calling this */
        ESP_LOGW(TAG, "ESP-NOW initialization requires WiFi to be started");
        ESP_LOGI(TAG, "Call network_tx_espnow_init() after WiFi start for hardware testing");
    }

    s_net_config.initialized = true;
    return ret;
}

/* Initialize ESP-NOW after WiFi is started (for hardware testing) */
esp_err_t network_tx_espnow_init(void)
{
    if (!s_net_config.use_espnow) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing ESP-NOW...");

    esp_err_t ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Add broadcast peer */
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, s_net_config.espnow_peer_mac, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;
    peer_info.ifidx = ESP_IF_WIFI_STA;

    ret = esp_now_add_peer(&peer_info);
    if (ret == ESP_OK || ret == ESP_ERR_ESPNOW_EXIST) {
        s_espnow_peer_added = true;
        ESP_LOGI(TAG, "ESP-NOW broadcast peer added");
    } else {
        ESP_LOGE(TAG, "Failed to add ESP-NOW peer: %s", esp_err_to_name(ret));
        esp_now_deinit();
        return ret;
    }

    /* Register send callback */
    esp_now_register_send_cb(network_tx_espnow_send_cb);

    return ESP_OK;
}

/* ESP-NOW send callback */
static void network_tx_espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "ESP-NOW send failed to " MACSTR, MAC2STR(tx_info->des_addr));
    }
}

/* Configure UDP destination */
void network_tx_set_udp_dest(const char *ip, uint16_t port)
{
    if (ip) {
        strncpy(s_net_config.udp_dst_ip, ip, sizeof(s_net_config.udp_dst_ip) - 1);
        s_net_config.udp_dst_ip[sizeof(s_net_config.udp_dst_ip) - 1] = '\0';
    }
    s_net_config.udp_dst_port = port;

    if (s_net_config.udp_socket >= 0) {
        memset(&s_net_config.udp_dest_addr, 0, sizeof(s_net_config.udp_dest_addr));
        s_net_config.udp_dest_addr.sin_family = AF_INET;
        s_net_config.udp_dest_addr.sin_port = htons(port);
        inet_pton(AF_INET, s_net_config.udp_dst_ip, &s_net_config.udp_dest_addr.sin_addr);
    }

    ESP_LOGI(TAG, "UDP destination set to %s:%d", s_net_config.udp_dst_ip, s_net_config.udp_dst_port);
}

/* Configure ESP-NOW peer MAC */
void network_tx_set_espnow_peer(const uint8_t *mac)
{
    if (mac) {
        memcpy(s_net_config.espnow_peer_mac, mac, 6);
        ESP_LOGI(TAG, "ESP-NOW peer set to " MACSTR, MAC2STR(mac));
    }
}

void network_tx_send(ts_packet_t *ts)
{
    if (!ts || !ts->data || ts->total_len == 0) {
        ESP_LOGW(TAG, "Invalid TS packet");
        return;
    }

    bool sent_any = false;

    /* Send via UDP */
    if (s_net_config.use_udp && s_net_config.udp_socket >= 0) {
        int sent = sendto(s_net_config.udp_socket, ts->data, ts->total_len, 0,
                          (struct sockaddr *)&s_net_config.udp_dest_addr,
                          sizeof(s_net_config.udp_dest_addr));
        if (sent > 0) {
            sent_any = true;
            ESP_LOGD(TAG, "UDP sent %d bytes to %s:%d", sent, s_net_config.udp_dst_ip, s_net_config.udp_dst_port);
        } else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            ESP_LOGW(TAG, "UDP send failed: errno %d", errno);
        }
    }

    /* Send via ESP-NOW */
    if (s_net_config.use_espnow && s_espnow_peer_added) {
        esp_err_t ret = esp_now_send(s_net_config.espnow_peer_mac, ts->data, ts->total_len);
        if (ret == ESP_OK) {
            sent_any = true;
            ESP_LOGD(TAG, "ESP-NOW sent %zu bytes", ts->total_len);
        } else {
            ESP_LOGW(TAG, "ESP-NOW send failed: %s", esp_err_to_name(ret));
        }
    }

    if (!sent_any) {
        ESP_LOGD(TAG, "No transport available, packet dropped (%zu bytes)", ts->total_len);
    }
}

void network_tx_deinit(void)
{
    if (s_net_config.use_espnow && s_espnow_peer_added) {
        esp_now_del_peer(s_net_config.espnow_peer_mac);
        s_espnow_peer_added = false;
        esp_now_deinit();
        ESP_LOGI(TAG, "ESP-NOW deinitialized");
    }

    if (s_net_config.use_udp && s_net_config.udp_socket >= 0) {
        close(s_net_config.udp_socket);
        s_net_config.udp_socket = -1;
        ESP_LOGI(TAG, "UDP socket closed");
    }

    s_net_config.initialized = false;
}