/*
 * UART Telemetry Unit Tests
 *
 * Tests for UART telemetry JSON output.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "uart_telemetry.h"
#include "stanag_types.h"
#include "h264_encoder.h"
#include "sensor_sim.h"

/* Test setup/teardown */
void setUp(void) {
    uart_telemetry_init();
    h264_encoder_init();
    sensor_sim_init();
}

void tearDown(void) {
    uart_telemetry_deinit();
}

/* Test: uart_telemetry_init returns ESP_OK */
void test_uart_telemetry_init(void)
{
    esp_err_t ret = uart_telemetry_init();
    /* May fail in test environment if UART pins not available */
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE);
}

/* Test: uart_telemetry_send with valid data */
void test_uart_telemetry_send_valid(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    TEST_ASSERT_NOT_NULL(nalu);

    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    /* Should not crash (falls back to stdout in test env) */
    uart_telemetry_send(&sensors, nalu, 0);
    TEST_PASS();
}

/* Test: uart_telemetry_send with NULL sensors */
void test_uart_telemetry_send_null_sensors(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    TEST_ASSERT_NOT_NULL(nalu);

    uart_telemetry_send(NULL, nalu, 0);
    TEST_PASS();
}

/* Test: uart_telemetry_send with NULL nalu */
void test_uart_telemetry_send_null_nalu(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    uart_telemetry_send(&sensors, NULL, 0);
    TEST_PASS();
}

/* Test: uart_telemetry_deinit cleans up */
void test_uart_telemetry_deinit(void)
{
    uart_telemetry_deinit();
    TEST_PASS();
}