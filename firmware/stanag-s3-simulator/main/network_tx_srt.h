/*
 * Network TX SRT - Secure Reliable Transport Implementation
 *
 * Sends MPEG-TS over SRT (Haivision SRT protocol)
 * Compatible with MediaMTX, ffmpeg, GStreamer, SRT Gateway
 *
 * Internal module functions (prefixed with _module_) are called by network_tx.c
 */

#ifndef NETWORK_TX_SRT_H
#define NETWORK_TX_SRT_H

#include "stanag_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SRT connection mode */
typedef enum {
    SRT_MODE_CALLER,   /* Connect to remote listener (most common) */
    SRT_MODE_LISTENER, /* Bind and wait for incoming connection */
    SRT_MODE_RENDEZVOUS /* Both sides connect to each other */
} srt_mode_t;

/* SRT configuration */
typedef struct {
    char ip[64];              /* Remote IP (caller) or bind IP (listener) */
    uint16_t port;            /* Remote port (caller) or bind port (listener) */
    srt_mode_t mode;          /* Connection mode */
    char passphrase[64];      /* Encryption passphrase (empty = no encryption) */
    int pbkeylen;             /* PB key length: 0, 16, 24, 32 (bytes) */
    int latency_ms;           /* SRT latency buffer (ms), default 200 */
    bool enable_tlpktdrop;    /* Drop packets too late for playout */
    int mss;                  /* Max segment size (bytes), default 1500 */
    bool nonce_enabled;       /* Use nonce for encryption */
    char stream_id[256];      /* SRT stream ID (for authentication/routing) */
} srt_config_t;

/* Internal module functions (called by network_tx.c) */
esp_err_t network_tx_srt_module_init(const srt_config_t *config);
esp_err_t network_tx_srt_module_send(ts_packet_t *ts);
esp_err_t network_tx_srt_module_get_stats(char *buf, size_t buf_len);
bool network_tx_srt_module_is_connected(void);
void network_tx_srt_module_deinit(void);

/* Default configuration helper */
void network_tx_srt_config_default(srt_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_TX_SRT_H