/*
 * UART Telemetry - Header File
 */

#ifndef UART_TELEMETRY_H
#define UART_TELEMETRY_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t uart_telemetry_init(void);
void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num);

#endif // UART_TELEMETRY_H