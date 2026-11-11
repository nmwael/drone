/*
 * KLV Builder - MISB ST 0601 KLV Metadata Builder
 */

#ifndef KLV_BUILDER_H
#define KLV_BUILDER_H

#include "stanag_types.h"
#include "esp_err.h"

/* MISB ST 0601 KLV Keys */
#define ST0601_KEY_POSITION_LLh   0x0001  /* Latitude/Longitude/height */
#define ST0601_KEY_VEL_NED        0x0002  /* Velocity North/East/Down */
#define ST0601_KEY_HEADING        0x0003  /* Heading */
#define ST0601_KEY_ACCEL          0x0004  /* Acceleration */
#define ST0601_KEY_ANG_RATE       0x0008  /* Angular Rate */
#define ST0601_KEY_BARO_ALT       0x0102  /* Barometric Altitude */
#define ST0601_KEY_TEMP           0x0103  /* Temperature */
#define ST0601_KEY_MAG_HEADING    0x0104  /* Magnetic Heading */

/* KLV encoding helpers */
#define KLV_KEY_SIZE 2       /* 2-byte key */
#define KLV_LEN_SIZE 2       /* 2-byte length */

esp_err_t klv_builder_init(void);
klv_packet_t* klv_builder_build(sensor_data_t *sensors, uint32_t frame_num);
void klv_packet_free(klv_packet_t *packet);

/* Internal: encode a single KLV element */
static inline uint8_t* klv_encode_key_length(uint8_t *buf, uint16_t key, uint16_t length);
static inline uint16_t klv_decode_key(const uint8_t *buf);

#endif // KLV_BUILDER_H