/*
 * Network TX - Header File
 */

#ifndef NETWORK_TX_H
#define NETWORK_TX_H

#include "stanag_types.h"
#include "esp_err.h"
#include "network_tx_srt.h"

/* Transport mode */
typedef enum {
    NETWORK_TX_MODE_UDP,
    NETWORK_TX_MODE_SRT,
    NETWORK_TX_MODE_RTMP,
    NETWORK_TX_MODE_ESP_NOW,
    NETWORK_TX_MODE_UNIX_SOCKET
} network_tx_mode_t;

/* Function Prototypes */

esp_err_t network_tx_init(void);
esp_err_t network_tx_espnow_init(void);  /* Call after WiFi start for hardware */
void network_tx_send(ts_packet_t *ts);
void network_tx_deinit(void);

/* Configuration */
void network_tx_set_mode(network_tx_mode_t mode);
void network_tx_set_udp_dest(const char *ip, uint16_t port);
void network_tx_set_espnow_peer(const uint8_t *mac);

/* Unix socket (QEMU/devcontainer testing) */
void network_tx_set_unix_socket(const char *path);

/* SRT Configuration - uses types from network_tx_srt.h */
void network_tx_set_srt_config(const srt_config_t *config);
void network_tx_srt_config_default(srt_config_t *config);

/* SRT Runtime */
esp_err_t network_tx_srt_init(void);  /* Call after network_tx_init() if using SRT */
bool network_tx_srt_is_connected(void);
esp_err_t network_tx_srt_get_stats(char *buf, size_t buf_len);
void network_tx_srt_deinit(void);     /* Tear down SRT for reconnect */

/* RTMP Configuration */
void network_tx_set_rtmp_config(const char *server, uint16_t port,
                                const char *app, const char *stream,
                                const char *user, const char *pass);
void network_tx_rtmp_config_default(void);

/* RTMP Runtime */
esp_err_t network_tx_rtmp_init(void);  /* Call after network_tx_init() if using RTMP */
bool network_tx_rtmp_is_connected(void);
void network_tx_rtmp_deinit(void);

/* Get current transport mode */
network_tx_mode_t network_tx_get_mode(void);

/* Creative network diagnostics - run when SRT connection fails */
void network_tx_print_netif_info(void);
void network_tx_run_udp_probe(const char *ip);
void network_tx_try_alternate_stream_ids(void);

#endif // NETWORK_TX_H