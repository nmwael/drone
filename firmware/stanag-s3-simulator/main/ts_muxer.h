/*
 * TS Muxer - Header File
 */

#ifndef TS_MUXER_H
#define TS_MUXER_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t ts_muxer_init(void);
ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv);

#endif // TS_MUXER_H