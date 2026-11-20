/*
 * Network TX RTMP - Header File
 */

#ifndef NETWORK_TX_RTMP_H
#define NETWORK_TX_RTMP_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RTMP Configuration */
void network_tx_set_rtmp_config(const char *server, uint16_t port,
                                const char *app, const char *stream,
                                const char *user, const char *pass);
void network_tx_rtmp_config_default(void);

/* RTMP Runtime */
esp_err_t network_tx_rtmp_init(void);   /* Call after network_tx_init() if using RTMP */
bool network_tx_rtmp_is_connected(void);
void network_tx_rtmp_deinit(void);      /* Tear down RTMP for reconnect */

/* Internal module init (called from network_tx.c wrapper) */
esp_err_t network_tx_rtmp_module_init(void);

/* Send TS packet data over RTMP as H.264 FLV video tags */
int network_tx_rtmp_send(const uint8_t *data, int len, uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_TX_RTMP_H */
