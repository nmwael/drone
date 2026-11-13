/*
 * Network TX - Header File
 */

#ifndef NETWORK_TX_H
#define NETWORK_TX_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t network_tx_init(void);
esp_err_t network_tx_espnow_init(void);  /* Call after WiFi start for hardware */
void network_tx_send(ts_packet_t *ts);
void network_tx_deinit(void);

/* Configuration */
void network_tx_set_udp_dest(const char *ip, uint16_t port);
void network_tx_set_espnow_peer(const uint8_t *mac);

/* Unix socket (QEMU/devcontainer testing) */
void network_tx_set_unix_socket(const char *path);

#endif // NETWORK_TX_H