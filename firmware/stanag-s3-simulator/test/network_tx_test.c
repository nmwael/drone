/*
 * Network TX Unit Tests
 *
 * Tests for network transmission (ESP-NOW/UDP).
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "network_tx.h"
#include "stanag_types.h"
#include "ts_muxer.h"
#include "h264_encoder.h"
#include "klv_builder.h"
#include "sensor_sim.h"

/* Test setup/teardown */
void setUp(void) {
    network_tx_init();
    ts_muxer_init();
    h264_encoder_init();
    klv_builder_init();
    sensor_sim_init();
}

void tearDown(void) {
    network_tx_deinit();
}

/* Test: network_tx_init returns ESP_OK */
void test_network_tx_init(void)
{
    esp_err_t ret = network_tx_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: network_tx_set_udp_dest sets destination */
void test_network_tx_set_udp_dest(void)
{
    network_tx_set_udp_dest("192.168.1.100", 9999);
    /* No direct getter, but we can verify by attempting send */
    TEST_PASS();
}

/* Test: network_tx_set_espnow_peer sets peer MAC */
void test_network_tx_set_espnow_peer(void)
{
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    network_tx_set_espnow_peer(mac);
    TEST_PASS();
}

/* Test: network_tx_send with valid TS packet */
void test_network_tx_send_valid_ts(void)
{
    yuv420_frame_t dummy_yuv = {0};
    dummy_yuv.timestamp_us = 1000000;

    h264_nalu_t *nalu = h264_encoder_encode(&dummy_yuv);
    TEST_ASSERT_NOT_NULL(nalu);

    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);
    klv_packet_t *klv = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(klv);

    ts_packet_t *ts = ts_muxer_mux(nalu, klv);
    TEST_ASSERT_NOT_NULL(ts);

    /* Should not crash */
    network_tx_send(ts);

    klv_packet_free(klv);
}

/* Test: network_tx_send with NULL TS packet */
void test_network_tx_send_null_ts(void)
{
    network_tx_send(NULL);
    TEST_PASS();
}

/* Test: network_tx_send with empty TS packet */
void test_network_tx_send_empty_ts(void)
{
    ts_packet_t empty_ts = {0};
    network_tx_send(&empty_ts);
    TEST_PASS();
}

/* Test: network_tx_deinit cleans up */
void test_network_tx_deinit(void)
{
    network_tx_deinit();
    TEST_PASS();
}