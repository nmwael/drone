/*
 * H.264 Encoder Unit Tests
 *
 * Tests for H.264 encoding.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "h264_encoder.h"
#include "stanag_types.h"

/* Test setup/teardown */
void setUp(void) {
    h264_encoder_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: h264_encoder_init returns ESP_OK */
void test_h264_encoder_init(void)
{
    esp_err_t ret = h264_encoder_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: h264_encoder_encode with valid YUV returns NALU */
void test_h264_encoder_encode_valid_yuv(void)
{
    yuv420_frame_t yuv = {0};
    yuv.width = 1280;
    yuv.height = 720;
    yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_NOT_NULL(nalu->data);
    TEST_ASSERT_GREATER_THAN(0, nalu->len);
    TEST_ASSERT_EQUAL(5, nalu->type);  /* IDR slice */
    TEST_ASSERT_TRUE(nalu->keyframe);
    TEST_ASSERT_EQUAL(yuv.timestamp_us, nalu->timestamp_us);
}

/* Test: h264_encoder_encode with NULL YUV returns NULL */
void test_h264_encoder_encode_null_yuv(void)
{
    h264_nalu_t *nalu = h264_encoder_encode(NULL);
    TEST_ASSERT_NULL(nalu);
}

/* Test: NALU starts with start code (0x00 0x00 0x00 0x01) */
void test_h264_nalu_start_code(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_GREATER_OR_EQUAL(4, nalu->len);

    TEST_ASSERT_EQUAL_HEX8(0x00, nalu->data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, nalu->data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, nalu->data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01, nalu->data[3]);
}

/* Test: NALU header indicates IDR slice (type 5) */
void test_h264_nalu_idr_type(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_GREATER_OR_EQUAL(5, nalu->len);

    /* NAL header: forbidden_bit(0) + nal_ref_idc(3) + nal_unit_type(5) = 0x65 */
    TEST_ASSERT_EQUAL_HEX8(0x65, nalu->data[4]);
}

/* Test: Timestamp is propagated from YUV to NALU */
void test_h264_timestamp_propagation(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 123456789;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_EQUAL(123456789, nalu->timestamp_us);
}

/* Test: Keyframe flag is true for IDR */
void test_h264_keyframe_flag(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_TRUE(nalu->keyframe);
}

/* Test: Multiple encodes return valid NALUs */
void test_h264_multiple_encodes(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 1000000;

    for (int i = 0; i < 10; i++) {
        yuv.timestamp_us = 1000000 + i * 33333;
        h264_nalu_t *nalu = h264_encoder_encode(&yuv);
        TEST_ASSERT_NOT_NULL(nalu);
        TEST_ASSERT_NOT_NULL(nalu->data);
        TEST_ASSERT_GREATER_THAN(0, nalu->len);
    }
}

/* Test: NALU type field matches expected */
void test_h264_nalu_type_field(void)
{
    yuv420_frame_t yuv = {0};
    yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&yuv);
    TEST_ASSERT_NOT_NULL(nalu);
    TEST_ASSERT_EQUAL(5, nalu->type);  /* IDR = 5 */
}