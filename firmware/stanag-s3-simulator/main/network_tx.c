/*
 * Network TX - Multi-Transport Implementation
 *
 * Supports UDP, SRT, ESP-NOW, and Unix socket for MPEG-TS streaming
 */

#include "stanag_types.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <fcntl.h>
#include <sys/un.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "network_tx.h"
#include "network_tx_srt.h"
#include "network_tx_rtmp.h"

/* MAC address formatting macros (from esp_rom/ets_sys.h) */
#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

static const char *TAG = "NETWORK_TX";

/* Active transport mode */
static network_tx_mode_t s_active_mode = NETWORK_TX_MODE_UDP;

/* SRT configuration */
static srt_config_t s_srt_config;

/* Configuration - set via menuconfig or runtime */
static struct {
    bool use_espnow;
    bool use_udp;
    bool use_unix_socket;
    uint8_t espnow_peer_mac[6];
    char udp_dst_ip[16];
    uint16_t udp_dst_port;
    int udp_socket;
    struct sockaddr_in udp_dest_addr;
    char unix_socket_path[128];
    int unix_socket_fd;
    bool initialized;
} s_net_config = {
    .use_espnow = true,
    .use_udp = true,
    .use_unix_socket = false,
    .espnow_peer_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* Broadcast */
    .udp_dst_ip = "192.168.4.2",  /* Default MediaMTX IP */
    .udp_dst_port = 8888,
    .udp_socket = -1,
    .unix_socket_path = "",
    .unix_socket_fd = -1,
    .initialized = false
};

static bool s_espnow_peer_added = false;

/* Forward declaration for ESP-NOW send callback */
static void network_tx_espnow_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);

void network_tx_set_mode(network_tx_mode_t mode)
{
    s_active_mode = mode;
    ESP_LOGI(TAG, "Transport mode set to %d", mode);
}



void network_tx_set_srt_config(const srt_config_t *config)
{
    if (config) {
        memcpy(&s_srt_config, config, sizeof(srt_config_t));
        ESP_LOGI(TAG, "SRT config updated: %s:%d, mode=%d, latency=%dms, encryption=%s",
                 s_srt_config.ip, s_srt_config.port, s_srt_config.mode,
                 s_srt_config.latency_ms,
                 s_srt_config.passphrase[0] ? "enabled" : "disabled");
    }
}

esp_err_t network_tx_init(void)
{
    ESP_LOGI(TAG, "Network TX initializing (mode: %d, ESP-NOW: %s, UDP: %s, Unix socket: %s)",
             s_active_mode,
             s_net_config.use_espnow ? "yes" : "no",
             s_net_config.use_udp ? "yes" : "no",
             s_net_config.use_unix_socket ? "yes" : "no");

    esp_err_t ret = ESP_OK;

    /* Initialize Unix socket if enabled */
    if (s_net_config.use_unix_socket && s_net_config.unix_socket_path[0] != '\0') {
        s_net_config.unix_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (s_net_config.unix_socket_fd < 0) {
            ESP_LOGE(TAG, "Failed to create Unix socket: errno %d", errno);
            s_net_config.use_unix_socket = false;
        } else {
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, s_net_config.unix_socket_path,
                    sizeof(addr.sun_path) - 1);

            if (connect(s_net_config.unix_socket_fd,
                        (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                ESP_LOGW(TAG, "Failed to connect to Unix socket %s: errno %d. "
                                "Falling back to UDP if enabled.",
                         s_net_config.unix_socket_path, errno);
                close(s_net_config.unix_socket_fd);
                s_net_config.unix_socket_fd = -1;
                s_net_config.use_unix_socket = false;
            } else {
                ESP_LOGI(TAG, "Unix socket connected to %s",
                         s_net_config.unix_socket_path);
            }
        }
    }

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

            ESP_LOGI(TAG, "UDP socket created, target: %s:%d",
                     s_net_config.udp_dst_ip, s_net_config.udp_dst_port);
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

/* Initialize SRT transport (call after network_tx_init if using SRT mode) */
esp_err_t network_tx_srt_init(void)
{
    if (s_active_mode != NETWORK_TX_MODE_SRT) {
        ESP_LOGW(TAG, "SRT init called but active mode is %d", s_active_mode);
        return ESP_ERR_INVALID_STATE;
    }
    /* Apply defaults if no config was set via network_tx_set_srt_config() */
    if (s_srt_config.ip[0] == '\0') {
        network_tx_srt_config_default(&s_srt_config);
    }
    return network_tx_srt_module_init(&s_srt_config);
}

/* Initialize RTMP transport (call after network_tx_init if using RTMP mode) */
esp_err_t network_tx_rtmp_init(void)
{
    if (s_active_mode != NETWORK_TX_MODE_RTMP) {
        ESP_LOGW(TAG, "RTMP init called but active mode is %d", s_active_mode);
        return ESP_ERR_INVALID_STATE;
    }
    return network_tx_rtmp_module_init();
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

/* Debug: print TS packet as hex to serial (for QEMU test-sender forwarding) */
static void network_tx_send_debug_hex(const uint8_t *data, size_t len)
{
    char hexbuf[2048];
    size_t pos = 0;
    size_t max_bytes = (sizeof(hexbuf) - 1) / 3;
    size_t dump_len = (len < max_bytes) ? len : max_bytes;

    for (size_t i = 0; i < dump_len; i++) {
        pos += snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "%02X", data[i]);
    }
    if (len > max_bytes) {
        snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "...");
    }
    ESP_LOGI(TAG, "TS_OUT: %s", hexbuf);
}

/* Configure Unix socket path for QEMU/devcontainer testing */
void network_tx_set_unix_socket(const char *path)
{
    if (path) {
        strncpy(s_net_config.unix_socket_path, path,
                sizeof(s_net_config.unix_socket_path) - 1);
        s_net_config.unix_socket_path[sizeof(s_net_config.unix_socket_path) - 1] = '\0';
        s_net_config.use_unix_socket = true;
        ESP_LOGI(TAG, "Unix socket path set to %s", s_net_config.unix_socket_path);
    }
}

void network_tx_send(ts_packet_t *ts)
{
    if (!ts || !ts->data || ts->total_len == 0) {
        ESP_LOGW(TAG, "Invalid TS packet");
        return;
    }

    bool sent_any = false;

    /* Send via SRT (if active mode) */
    if (s_active_mode == NETWORK_TX_MODE_SRT) {
        esp_err_t ret = network_tx_srt_module_send(ts);
        if (ret == ESP_OK) {
            sent_any = true;
        } else if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "SRT not connected, packet dropped");
        }
    }

    /* Send via RTMP (if active mode) */
    if (s_active_mode == NETWORK_TX_MODE_RTMP) {
        int ret = network_tx_rtmp_send(ts->data, ts->total_len, 0);
        if (ret >= 0) {
            sent_any = true;
        } else {
            ESP_LOGW(TAG, "RTMP not connected, packet dropped");
        }
    }

    /* Send via Unix socket (QEMU/devcontainer testing) */
    if (s_active_mode == NETWORK_TX_MODE_UNIX_SOCKET ||
        (s_net_config.use_unix_socket && s_net_config.unix_socket_fd >= 0)) {
        int sent = sendto(s_net_config.unix_socket_fd, ts->data, ts->total_len, 0,
                          NULL, 0);
        if (sent > 0) {
            sent_any = true;
            ESP_LOGD(TAG, "Unix socket sent %d bytes to %s",
                     sent, s_net_config.unix_socket_path);
        } else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            ESP_LOGW(TAG, "Unix socket send failed: errno %d", errno);
        }
        /* Also print hex to serial for test-sender forwarding (QEMU can't access host Unix socket) */
        network_tx_send_debug_hex(ts->data, ts->total_len);
    }

    /* Send via UDP */
    if (s_active_mode == NETWORK_TX_MODE_UDP ||
        (s_net_config.use_udp && s_net_config.udp_socket >= 0)) {
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
    if (s_active_mode == NETWORK_TX_MODE_ESP_NOW ||
        (s_net_config.use_espnow && s_espnow_peer_added)) {
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

bool network_tx_srt_is_connected(void)
{
    return network_tx_srt_module_is_connected();
}

void network_tx_srt_deinit(void)
{
    network_tx_srt_module_deinit();
}

esp_err_t network_tx_srt_get_stats(char *buf, size_t buf_len)
{
    return network_tx_srt_module_get_stats(buf, buf_len);
}

network_tx_mode_t network_tx_get_mode(void)
{
    return s_active_mode;
}

void network_tx_deinit(void)
{
    /* Deinit SRT if active */
    if (s_active_mode == NETWORK_TX_MODE_SRT) {
        network_tx_srt_module_deinit();
    }

    /* Deinit RTMP if active */
    if (s_active_mode == NETWORK_TX_MODE_RTMP) {
        network_tx_rtmp_deinit();
    }

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

    if (s_net_config.use_unix_socket && s_net_config.unix_socket_fd >= 0) {
        close(s_net_config.unix_socket_fd);
        s_net_config.unix_socket_fd = -1;
        ESP_LOGI(TAG, "Unix socket closed");
    }

    s_net_config.initialized = false;
}

/* ------------------------------------------------------------------ */
/* Creative Network Diagnostics                                       */
/* Helps diagnose why 172.160.27.93:8890 doesn't respond to ESP32     */
/* ------------------------------------------------------------------ */

/* Ports to probe on the remote SRT endpoint */
static const uint16_t s_probe_ports[] = {8890, 8888, 1935, 8554, 8889, 554};
#define S_PROBE_PORTS_COUNT (sizeof(s_probe_ports) / sizeof(s_probe_ports[0]))

/* Print ESP32 network interface info (IP, gateway, netmask, MAC) */
void network_tx_print_netif_info(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        ESP_LOGW(TAG, "Cannot get WiFi STA netif handle");
        return;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "=== ESP32 Network Interface ===");
        ESP_LOGI(TAG, "IP:    " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI(TAG, "Mask:  " IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI(TAG, "GW:    " IPSTR, IP2STR(&ip_info.gw));
        ESP_LOGI(TAG, "=================================");
    }

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "MAC: " MACSTR, MAC2STR(mac));
}

/* Send a raw UDP probe packet to a specific port.
   Returns >0 if we get ANY response (including ICMP port unreachable),
   0 if timeout, <0 on error. */
int network_tx_udp_probe_port(const char *ip, uint16_t port, int timeout_ms)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        ESP_LOGE(TAG, "probe: socket() failed: errno %d", errno);
        return -1;
    }

    /* Set receive timeout */
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    inet_pton(AF_INET, ip, &dst.sin_addr);

    const char *probe = "SRT_PROBE";
    int sent = sendto(fd, probe, 9, 0, (struct sockaddr *)&dst, sizeof(dst));
    if (sent < 0) {
        ESP_LOGW(TAG, "probe port %d: sendto failed: errno %d", port, errno);
        close(fd);
        return -1;
    }

    /* Try to receive any response (ICMP port unreachable, ACK, etc.) */
    uint8_t buf[256];
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);
    int r = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&src, &srclen);
    close(fd);

    if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  /* timeout - no response */
        }
        return -1;
    }
    return r;  /* got some response */
}

/* Run a full UDP port probe against the remote endpoint.
   This tells us if the host is reachable at all, or if a firewall
   is dropping all packets. */
void network_tx_run_udp_probe(const char *ip)
{
    ESP_LOGW(TAG, "=== UDP Port Probe: %s ===", ip);
    ESP_LOGW(TAG, "Probing ports: %d, %d, %d, %d, %d, %d",
             s_probe_ports[0], s_probe_ports[1], s_probe_ports[2],
             s_probe_ports[3], s_probe_ports[4], s_probe_ports[5]);

    int reachable_count = 0;
    for (int i = 0; i < S_PROBE_PORTS_COUNT; i++) {
        uint16_t port = s_probe_ports[i];
        int r = network_tx_udp_probe_port(ip, port, 500);

        if (r > 0) {
            ESP_LOGW(TAG, "  Port %d: GOT RESPONSE (%d bytes) - HOST IS REACHABLE!", port, r);
            reachable_count++;
        } else if (r == 0) {
            ESP_LOGW(TAG, "  Port %d: TIMEOUT - firewall dropping or no service", port);
        } else {
            ESP_LOGW(TAG, "  Port %d: ERROR", port);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (reachable_count > 0) {
        ESP_LOGW(TAG, "=== RESULT: %d port(s) responded - host IS reachable ===", reachable_count);
    } else {
        ESP_LOGW(TAG, "=== RESULT: NO ports responded - host may be firewalled or unreachable ===");
    }
    ESP_LOGW(TAG, "===============================================================");
}

/* Try alternative stream ID formats that MediaMTX might accept */
void network_tx_try_alternate_stream_ids(void)
{
    ESP_LOGW(TAG, "=== Alternative Stream ID Formats ===");
    ESP_LOGW(TAG, "Current:  publish:live/sse_drone_f71919:sse_drone_f71919:5eefeb2389cf8b2ac83218ae926e7143");
    ESP_LOGW(TAG, "Alt 1:    publish:stanag");
    ESP_LOGW(TAG, "Alt 2:    publish:live/stanag");
    ESP_LOGW(TAG, "Alt 3:    publish:drone");
    ESP_LOGW(TAG, "Alt 4:    publish:live/sse_drone_f71919");
    ESP_LOGW(TAG, "===============================================================");
}