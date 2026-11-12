/*
 * JPEG Loader Unit Tests
 *
 * Tests for JPEG loading and decoding.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "jpeg_loader.h"
#include "stanag_types.h"

/* Test setup/teardown */
void setUp(void) {
    jpeg_loader_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: jpeg_loader_init returns ESP_OK */
void test_jpeg_loader_init(void)
{
    esp_err_t ret = jpeg_loader_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: jpeg_loader_get_next returns valid frame */
void test_jpeg_loader_get_next(void)
{
    jpeg_frame_t *frame = jpeg_loader_get_next();
    TEST_ASSERT_NOT_NULL(frame);
    TEST_ASSERT_NOT_NULL(frame->data);
    TEST_ASSERT_GREATER_THAN(0, frame->len);
    TEST_ASSERT_EQUAL(1280, frame->width);
    TEST_ASSERT_EQUAL(720, frame->height);
    TEST_ASSERT_GREATER_OR_EQUAL(0, frame->index);
    TEST_ASSERT_LESS_THAN(5, frame->index);
}

/* Test: jpeg_loader_get_next cycles through frames */
void test_jpeg_loader_frame_cycling(void)
{
    int indices[10];
    for (int i = 0; i < 10; i++) {
        jpeg_frame_t *frame = jpeg_loader_get_next();
        TEST_ASSERT_NOT_NULL(frame);
        indices[i] = frame->index;
    }

    /* Should cycle 0,1,2,3,4,0,1,2,3,4 */
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(i % 5, indices[i]);
    }
}

/* Test: jpeg_loader_decode returns valid YUV frame */
void test_jpeg_loader_decode(void)
{
    jpeg_frame_t *frame = jpeg_loader_get_next();
    TEST_ASSERT_NOT_NULL(frame);

    yuv420_frame_t *yuv = jpeg_loader_decode(frame);
    TEST_ASSERT_NOT_NULL(yuv);
    TEST_ASSERT_NOT_NULL(yuv->y);
    TEST_ASSERT_NOT_NULL(yuv->u);
    TEST_ASSERT_NOT_NULL(yuv->v);
    TEST_ASSERT_EQUAL(1280, yuv->width);
    TEST_ASSERT_EQUAL(720, yuv->height);
    TEST_ASSERT_EQUAL(1280, yuv->stride_y);
    TEST_ASSERT_EQUAL(640, yuv->stride_uv);
    TEST_ASSERT_GREATER_THAN(0, yuv->timestamp_us);
}

/* Test: jpeg_loader_decode with NULL frame returns NULL */
void test_jpeg_loader_decode_null_frame(void)
{
    yuv420_frame_t *yuv = jpeg_loader_decode(NULL);
    TEST_ASSERT_NULL(yuv);
}

/* Test: jpeg_loader_decode with empty frame returns NULL */
void test_jpeg_loader_decode_empty_frame(void)
{
    jpeg_frame_t empty_frame = {0};
    yuv420_frame_t *yuv = jpeg_loader_decode(&empty_frame);
    TEST_ASSERT_NULL(yuv);
}

/* Test: YUV buffer layout is correct (I420 planar) */
void test_yuv_buffer_layout(void)
{
    jpeg_frame_t *frame = jpeg_loader_get_next();
    yuv420_frame_t *yuv = jpeg_loader_decode(frame);

    TEST_ASSERT_NOT_NULL(yuv);

    /* Y plane: width * height */
    size_t y_size = yuv->width * yuv->height;
    /* U plane: width/2 * height/2 */
    size_t uv_size = (yuv->width / 2) * (yuv->height / 2);

    /* Check U and V pointers are correctly offset */
    TEST_ASSERT_EQUAL_PTR(yuv->y + y_size, yuv->u);
    TEST_ASSERT_EQUAL_PTR(yuv->u + uv_size, yuv->v);

    /* Check strides */
    TEST_ASSERT_EQUAL(yuv->width, yuv->stride_y);
    TEST_ASSERT_EQUAL(yuv->width / 2, yuv->stride_uv);
}

/* Test: YUV test pattern (Y=gradient, U=128, V=128) */
void test_yuv_test_pattern(void)
{
    jpeg_frame_t *frame = jpeg_loader_get_next();
    yuv420_frame_t *yuv = jpeg_loader_decode(frame);

    TEST_ASSERT_NOT_NULL(yuv);

    /* Y should be gradient 0-255 */
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_HEX8(i, yuv->y[i]);
    }

    /* U should be 128 */
    for (int i = 0; i < (1280 * 720 / 4); i++) {
        TEST_ASSERT_EQUAL_HEX8(0x80, yuv->u[i]);
        TEST_ASSERT_EQUAL_HEX8(0x80, yuv->v[i]);
    }
}

/* Test: jpeg_loader_free_yuv handles NULL gracefully */
void test_jpeg_loader_free_yuv_null(void)
{
    jpeg_loader_free_yuv(NULL);  /* Should not crash */
    TEST_PASS();
}

/* Test: Multiple decode calls return same static buffer */
void test_jpeg_loader_static_buffer(void)
{
    jpeg_frame_t *frame1 = jpeg_loader_get_next();
    jpeg_frame_t *frame2 = jpeg_loader_get_next();

    yuv420_frame_t *yuv1 = jpeg_loader_decode(frame1);
    yuv420_frame_t *yuv2 = jpeg_loader_decode(frame2);

    /* Should return same static buffer */
    TEST_ASSERT_EQUAL_PTR(yuv1, yuv2);
    TEST_ASSERT_EQUAL_PTR(yuv1->y, yuv2->y);
}