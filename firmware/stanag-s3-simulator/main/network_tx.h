/*
 * Network TX - Header File
 */

#ifndef NETWORK_TX_H
#define NETWORK_TX_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t network_tx_init(void);
void network_tx_send(ts_packet_t *ts);

#endif // NETWORK_TX_H