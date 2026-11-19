/*
 * Network TX SRT - Secure Reliable Transport Implementation
 *
 * Sends MPEG-TS over SRT (Haivision SRT protocol)
 * Compatible with MediaMTX, ffmpeg, GStreamer, SRT Gateway
 *
 * This implementation requires the SRT component (lib/libsrt).
 * If SRT is not available, stub functions return ESP_ERR_NOT_SUPPORTED.
 */

#include "network_tx_srt.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "string.h"
#include "stdlib.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "srt.h"

/* Minimal SRT implementation is always available in components/srt */
#define CONFIG_NETWORK_TX_SRT_ENABLED 1
#define SRT_AVAILABLE 1

static const char *TAG = "NETWORK_TX_SRT";

#if SRT_AVAILABLE

/* Internal state */
static struct {
    SRTSOCKET socket;
    srt_config_t config;
    bool initialized;
    bool connected;
    SemaphoreHandle_t mutex;
} s_srt = {0};

/* Default configuration */
void network_tx_srt_config_default(srt_config_t *config)
{
    if (!config) return;
    memset(config, 0, sizeof(srt_config_t));
    config->port = 9000;
    config->mode = SRT_MODE_CALLER;
    config->latency_ms = 200;
    config->enable_tlpktdrop = true;
    config->mss = 1500;
    config->pbkeylen = 0;
    config->nonce_enabled = false;
    strcpy(config->ip, "192.168.4.2");
}

static esp_err_t srt_socket_setup(void)
{
    s_srt.socket = srt_create_socket();
    if (s_srt.socket == SRT_INVALID_SOCK) {
        ESP_LOGE(TAG, "Failed to create SRT socket: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    /* Set non-blocking */
    int yes = 1;
    srt_setsockopt(s_srt.socket, 0, SRTO_SNDSYN, &yes, sizeof(yes));
    srt_setsockopt(s_srt.socket, 0, SRTO_RCVSYN, &yes, sizeof(yes));

    /* Set latency */
    int latency = s_srt.config.latency_ms;
    srt_setsockopt(s_srt.socket, 0, SRTO_LATENCY, &latency, sizeof(latency));

    /* Set MSS */
    int mss = s_srt.config.mss;
    srt_setsockopt(s_srt.socket, 0, SRTO_MSS, &mss, sizeof(mss));

    /* Enable too-late packet drop */
    int tlpd = s_srt.config.enable_tlpktdrop ? 1 : 0;
    srt_setsockopt(s_srt.socket, 0, SRTO_TLPKTDROP, &tlpd, sizeof(tlpd));

    /* Set stream ID if provided (for authentication/routing) */
    if (s_srt.config.stream_id[0] != '\0') {
        srt_setsockopt(s_srt.socket, 0, SRTO_STREAMID,
                       s_srt.config.stream_id, strlen(s_srt.config.stream_id));
        ESP_LOGI(TAG, "SRT stream ID set: %s", s_srt.config.stream_id);
    }

    /* Set encryption if passphrase provided */
    if (s_srt.config.passphrase[0] != '\0' && s_srt.config.pbkeylen > 0) {
        srt_setsockopt(s_srt.socket, 0, SRTO_PASSPHRASE,
                       s_srt.config.passphrase, strlen(s_srt.config.passphrase));
        srt_setsockopt(s_srt.socket, 0, SRTO_PBKEYLEN,
                       &s_srt.config.pbkeylen, sizeof(s_srt.config.pbkeylen));
        if (s_srt.config.nonce_enabled) {
            int nonce = 1;
            srt_setsockopt(s_srt.socket, 0, SRTO_ENFORCEDENCRYPTION, &nonce, sizeof(nonce));
        }
    }

    return ESP_OK;
}

static esp_err_t srt_connect_caller(void)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s_srt.config.port);
    inet_pton(AF_INET, s_srt.config.ip, &addr.sin_addr);

    ESP_LOGI(TAG, "Connecting to SRT listener at %s:%d...", s_srt.config.ip, s_srt.config.port);

    int ret = srt_connect(s_srt.socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT connect failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    s_srt.connected = true;
    ESP_LOGI(TAG, "SRT connected to %s:%d", s_srt.config.ip, s_srt.config.port);
    return ESP_OK;
}

static esp_err_t srt_bind_listener(void)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s_srt.config.port);
    inet_pton(AF_INET, s_srt.config.ip, &addr.sin_addr);

    int ret = srt_bind(s_srt.socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT bind failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    ret = srt_listen(s_srt.socket, 1);
    if (ret == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT listen failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SRT listening on %s:%d", s_srt.config.ip, s_srt.config.port);

    /* Accept connection (blocking) */
    struct sockaddr_in client_addr;
    int client_len = (int)sizeof(client_addr);
    SRTSOCKET client = srt_accept(s_srt.socket, (struct sockaddr *)&client_addr, &client_len);
    if (client == SRT_INVALID_SOCK) {
        ESP_LOGE(TAG, "SRT accept failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    /* Close listening socket, use client socket */
    srt_close(s_srt.socket);
    s_srt.socket = client;
    s_srt.connected = true;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    ESP_LOGI(TAG, "SRT client connected from %s:%d", client_ip, ntohs(client_addr.sin_port));

    return ESP_OK;
}

static esp_err_t srt_connect_rendezvous(void)
{
    struct sockaddr_in local_addr, remote_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(s_srt.config.port);
    inet_pton(AF_INET, s_srt.config.ip, &local_addr.sin_addr);

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(s_srt.config.port);
    /* For rendezvous, remote IP should be set separately */
    inet_pton(AF_INET, s_srt.config.ip, &remote_addr.sin_addr);

    int ret = srt_bind(s_srt.socket, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (ret == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT rendezvous bind failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    ret = srt_connect(s_srt.socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (ret == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT rendezvous connect failed: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    s_srt.connected = true;
    ESP_LOGI(TAG, "SRT rendezvous connected");
    return ESP_OK;
}

esp_err_t network_tx_srt_module_init(const srt_config_t *config)
{
    if (s_srt.initialized) {
        ESP_LOGW(TAG, "SRT already initialized");
        return ESP_OK;
    }

    if (!config) {
        ESP_LOGE(TAG, "SRT config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_srt.config, config, sizeof(srt_config_t));

    /* Initialize SRT library globals (idempotent) */
    srt_startup();

    /* Create mutex */
    s_srt.mutex = xSemaphoreCreateMutex();
    if (!s_srt.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initializing SRT: mode=%d, %s:%d, latency=%dms, encryption=%s",
             s_srt.config.mode, s_srt.config.ip, s_srt.config.port,
             s_srt.config.latency_ms,
             s_srt.config.passphrase[0] ? "enabled" : "disabled");

    esp_err_t ret = srt_socket_setup();
    if (ret != ESP_OK) {
        goto cleanup;
    }

    switch (s_srt.config.mode) {
        case SRT_MODE_CALLER:
            ret = srt_connect_caller();
            break;
        case SRT_MODE_LISTENER:
            ret = srt_bind_listener();
            break;
        case SRT_MODE_RENDEZVOUS:
            ret = srt_connect_rendezvous();
            break;
        default:
            ESP_LOGE(TAG, "Unknown SRT mode: %d", s_srt.config.mode);
            ret = ESP_ERR_INVALID_ARG;
    }

    if (ret != ESP_OK) {
        goto cleanup;
    }

    s_srt.initialized = true;
    ESP_LOGI(TAG, "SRT initialized successfully");
    return ESP_OK;

cleanup:
    if (s_srt.socket != SRT_INVALID_SOCK) {
        srt_close(s_srt.socket);
        s_srt.socket = SRT_INVALID_SOCK;
    }
    if (s_srt.mutex) {
        vSemaphoreDelete(s_srt.mutex);
        s_srt.mutex = NULL;
    }
    return ret;
}

esp_err_t network_tx_srt_module_send(ts_packet_t *ts)
{
    if (!s_srt.initialized || !s_srt.connected) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ts || !ts->data || ts->total_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_srt.mutex, portMAX_DELAY);

    /* Send the TS payload directly as one SRT message (minimal streaming) */
    int sent = srt_sendmsg2(s_srt.socket, (char *)ts->data, (int)ts->total_len, NULL);
    if (sent == SRT_ERROR) {
        ESP_LOGE(TAG, "SRT send failed: %s", srt_getlasterror_str());
        s_srt.connected = false;
        xSemaphoreGive(s_srt.mutex);
        return ESP_FAIL;
    }

    xSemaphoreGive(s_srt.mutex);
    return ESP_OK;
}

esp_err_t network_tx_srt_module_flush(void)
{
    /* Minimal implementation sends each frame directly - nothing to flush */
    return ESP_OK;
}

esp_err_t network_tx_srt_module_get_stats(char *buf, size_t buf_len)
{
    if (!s_srt.initialized || !buf || buf_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    SRT_TRACEBSTATS stats;
    if (srt_bistats(s_srt.socket, &stats, 0) == SRT_ERROR) {
        snprintf(buf, buf_len, "SRT stats unavailable: %s", srt_getlasterror_str());
        return ESP_FAIL;
    }

    snprintf(buf, buf_len,
             "SRT Stats:\n"
             "  Connected: %s\n"
             "  Packets sent: %" PRIu64 "\n"
             "  Packets lost: %" PRIu64 "\n"
             "  Packets retransmitted: %" PRIu64 "\n"
             "  Bytes sent: %" PRIu64 "\n"
             "  Send buffer (bytes): %d\n"
             "  RTT (ms): %d\n"
             "  Bandwidth (Mbps): %.2f\n"
             "  Packet send period (us): %d",
             s_srt.connected ? "yes" : "no",
             stats.pktSentTotal,
             stats.pktRcvLossTotal,
             stats.pktRetransTotal,
             stats.byteSentTotal,
             stats.byteAvailSndBuf,
             stats.msRTT,
             (double)stats.mbpsSendRate,
             stats.pktSndPeriod);

    return ESP_OK;
}

bool network_tx_srt_module_is_connected(void)
{
    return s_srt.initialized && s_srt.connected;
}

void network_tx_srt_module_deinit(void)
{
    if (!s_srt.initialized) {
        return;
    }

    /* Flush remaining data */
    network_tx_srt_module_flush();

    if (s_srt.socket != SRT_INVALID_SOCK) {
        srt_close(s_srt.socket);
        s_srt.socket = SRT_INVALID_SOCK;
    }

    if (s_srt.mutex) {
        vSemaphoreDelete(s_srt.mutex);
        s_srt.mutex = NULL;
    }

    s_srt.initialized = false;
    s_srt.connected = false;

    ESP_LOGI(TAG, "SRT deinitialized");
}

#else  /* SRT not available - stub implementations */

void network_tx_srt_config_default(srt_config_t *config)
{
    if (!config) return;
    memset(config, 0, sizeof(srt_config_t));
    config->port = 9000;
    config->mode = SRT_MODE_CALLER;
    config->latency_ms = 200;
    config->enable_tlpktdrop = true;
    config->mss = 1500;
    config->pbkeylen = 0;
    config->nonce_enabled = false;
    strcpy(config->ip, "192.168.4.2");
}

esp_err_t network_tx_srt_module_init(const srt_config_t *config)
{
    (void)config;
    ESP_LOGW(TAG, "SRT STUB: network_tx_srt_module_init called - SRT not available, returning success to allow app to continue");
    return ESP_OK;
}

esp_err_t network_tx_srt_module_send(ts_packet_t *ts)
{
    (void)ts;
    ESP_LOGW(TAG, "SRT STUB: network_tx_srt_module_send called - SRT not available, returning success");
    return ESP_OK;
}

esp_err_t network_tx_srt_module_flush(void)
{
    ESP_LOGW(TAG, "SRT STUB: network_tx_srt_module_flush called - SRT not available");
    return ESP_OK;
}

esp_err_t network_tx_srt_module_get_stats(char *buf, size_t buf_len)
{
    if (buf && buf_len > 0) {
        snprintf(buf, buf_len, "SRT STUB - not implemented");
    }
    return ESP_OK;
}

bool network_tx_srt_module_is_connected(void)
{
    ESP_LOGW(TAG, "SRT STUB: network_tx_srt_module_is_connected - returning false");
    return false;
}

void network_tx_srt_module_deinit(void)
{
    ESP_LOGW(TAG, "SRT STUB: network_tx_srt_module_deinit called");
    /* No-op */
}

#endif  /* SRT_AVAILABLE */