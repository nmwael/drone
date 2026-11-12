/*
 * TS Muxer Unit Tests
 *
 * Tests for MPEG-TS packetization.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "ts_muxer.h"
#include "stanag_types.h"
#include "h264_encoder.h"
#include "klv_builder.h"
#include "sensor_sim.h"

/* Test setup/teardown */
void setUp(void) {
    ts_muxer_init();
    h264_encoder_init();
    klv_builder_init();
    sensor_sim_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: ts_muxer_init returns ESP_OK */
void test_ts_muxer_init(void)
{
    esp_err_t ret = ts_muxer_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: ts_muxer_mux with valid NALU returns TS packet */
void test_ts_muxer_mux_valid_nalu(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    TEST_ASSERT_NOT_NULL(nalu);

    klv_packet_t *klv = NULL;  /* No KLV for this test */

    ts_packet_t *ts = ts_muxer_mux(nalu, klv);
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_NOT_NULL(ts->data);
    TEST_ASSERT_GREATER_THAN(0, ts->count);
    TEST_ASSERT_EQUAL(ts->count * 188, ts->total_len);
    TEST_ASSERT_EQUAL(nalu->timestamp_us, ts->timestamp_us);
}

/* Test: ts_muxer_mux with NULL NALU returns NULL */
void test_ts_muxer_mux_null_nalu(void)
{
    ts_packet_t *ts = ts_muxer_mux(NULL, NULL);
    TEST_ASSERT_NULL(ts);
}

/* Test: TS packet has correct sync byte (0x47) */
void test_ts_packet_sync_byte(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_GREATER_OR_EQUAL(188, ts->total_len);

    /* First packet should start with 0x47 */
    TEST_ASSERT_EQUAL_HEX8(0x47, ts->data[0]);

    /* If multiple packets, each should start with 0x47 */
    for (size_t i = 1; i < ts->count; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x47, ts->data[i * 188]);
    }
}

/* Test: Video PID is 0x100 */
void test_ts_video_pid(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);

    /* First packet PID should be 0x100 */
    uint16_t pid = ((ts->data[1] & 0x1F) << 8) | ts->data[2];
    TEST_ASSERT_EQUAL_HEX16(0x100, pid);
}

/* Test: KLV PID is 0x101 when KLV present */
void test_ts_klv_pid(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);

    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);
    klv_packet_t *klv = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(klv);

    ts_packet_t *ts = ts_muxer_mux(nalu, klv);
    TEST_ASSERT_NOT_NULL(ts);

    /* Should have at least 2 packets (video + KLV) */
    TEST_ASSERT_GREATER_OR_EQUAL(2, ts->count);

    /* Find KLV packet (PID 0x101) */
    bool found_klv = false;
    for (size_t i = 0; i < ts->count; i++) {
        uint8_t *pkt = &ts->data[i * 188];
        uint16_t pid = ((pkt[1] & 0x1F) << 8) | pkt[2];
        if (pid == 0x101) {
            found_klv = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_klv);

    klv_packet_free(klv);
}

/* Test: Continuity counter increments per PID */
void test_ts_continuity_counter(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);

    /* First mux */
    ts_packet_t *ts1 = ts_muxer_mux(nalu, NULL);
    TEST_ASSERT_NOT_NULL(ts1);
    uint8_t cc1 = ts1->data[3] & 0x0F;

    /* Second mux */
    ts_packet_t *ts2 = ts_muxer_mux(nalu, NULL);
    TEST_ASSERT_NOT_NULL(ts2);
    uint8_t cc2 = ts2->data[3] & 0x0F;

    /* CC should increment (mod 16) */
    TEST_ASSERT_EQUAL_HEX8((cc1 + 1) & 0x0F, cc2);
}

/* Test: Payload unit start indicator set on first packet */
void test_ts_pusi_set(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);

    /* First packet should have PUSI = 1 */
    bool pusi = (ts->data[1] & 0x40) != 0;
    TEST_ASSERT_TRUE(pusi);
}

/* Test: Adaptation field control is 0x10 (no adaptation field) */
void test_ts_adaptation_field(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);

    /* Adaptation field control should be 0x10 (payload only, no adaptation) */
    uint8_t afc = (ts->data[3] & 0x30) >> 4;
    TEST_ASSERT_EQUAL_HEX8(0x01, afc);  /* 01 = payload only */
}

/* Test: Packet size is exactly 188 bytes */
void test_ts_packet_size(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_EQUAL(ts->count * 188, ts->total_len);

    /* Each packet should be accessible as 188-byte chunk */
    for (size_t i = 0; i < ts->count; i++) {
        uint8_t *pkt = &ts->data[i * 188];
        TEST_ASSERT_EQUAL_HEX8(0x47, pkt[0]);
    }
}

/* Test: Null packet (PID 0x1FFF) added when needed */
void test_ts_null_packet(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    ts_packet_t *ts = ts_muxer_mux(nalu, NULL);

    TEST_ASSERT_NOT_NULL(ts);

    /* Should have 3 packets (video + KLV placeholder + null) */
    TEST_ASSERT_EQUAL(3, ts->count);

    /* Third packet should be null packet (PID 0x1FFF) */
    uint8_t *pkt3 = &ts->data[2 * 188];
    uint16_t pid = ((pkt3[1] & 0x1F) << 8) | pkt3[2];
    TEST_ASSERT_EQUAL_HEX16(0x1FFF, pid);
}

/* Test: ts_packet_free handles NULL gracefully */
void test_ts_packet_free_null(void)
{
    ts_packet_free(NULL);  /* Should not crash */
    TEST_PASS();
}