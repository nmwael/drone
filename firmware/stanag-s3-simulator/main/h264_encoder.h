/*
 * H.264 Encoder - Header File
 */

#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t h264_encoder_init(void);
h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv);
esp_err_t h264_encoder_deinit(void);
void h264_encoder_reset_sps_pps(void);  /* Reset SPS/PPS sent flag for RTMP reconnect */

#endif // H264_ENCODER_H