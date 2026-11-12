/*
 * KLV Builder Unit Tests
 *
 * Tests for MISB ST 0601 KLV metadata encoding.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "klv_builder.h"
#include "stanag_types.h"
#include "sensor_sim.h"

/* Test setup/teardown */
void setUp(void) {
    klv_builder_init();
    sensor_sim_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: klv_builder_init returns ESP_OK */
void test_klv_builder_init(void)
{
    esp_err_t ret = klv_builder_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: klv_builder_build with valid sensor data returns non-NULL packet */
void test_klv_builder_build_valid_data(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(packet);
    TEST_ASSERT_NOT_NULL(packet->data);
    TEST_ASSERT_GREATER_THAN(0, packet->len);
    TEST_ASSERT_GREATER_THAN(0, packet->timestamp_us);

    klv_packet_free(packet);
}

/* Test: klv_builder_build with NULL sensors returns NULL */
void test_klv_builder_build_null_sensors(void)
{
    klv_packet_t *packet = klv_builder_build(NULL, 0);
    TEST_ASSERT_NULL(packet);
}

/* Test: KLV packet contains all expected keys */
void test_klv_packet_contains_expected_keys(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(packet);

    /* Parse KLV elements manually */
    uint8_t *ptr = packet->data;
    size_t remaining = packet->len;
    int key_count = 0;
    bool found_position = false;
    bool found_velocity = false;
    bool found_heading = false;
    bool found_baro_alt = false;
    bool found_temp = false;
    bool found_mag_heading = false;
    bool found_timestamp = false;

    while (remaining >= 4) {
        uint16_t key = (ptr[0] << 8) | ptr[1];
        uint16_t length = (ptr[2] << 8) | ptr[3];

        if (remaining < 4 + length) {
            break;
        }

        key_count++;
        switch (key) {
            case 0x0001: found_position = true; TEST_ASSERT_EQUAL(12, length); break;
            case 0x0002: found_velocity = true; TEST_ASSERT_EQUAL(12, length); break;
            case 0x0003: found_heading = true; TEST_ASSERT_EQUAL(4, length); break;
            case 0x0102: found_baro_alt = true; TEST_ASSERT_EQUAL(4, length); break;
            case 0x0103: found_temp = true; TEST_ASSERT_EQUAL(2, length); break;
            case 0x0104: found_mag_heading = true; TEST_ASSERT_EQUAL(4, length); break;
            case 0xFFFF: found_timestamp = true; TEST_ASSERT_EQUAL(4, length); break;
            default: break;
        }

        ptr += 4 + length;
        remaining -= 4 + length;
    }

    TEST_ASSERT_EQUAL(7, key_count);
    TEST_ASSERT_TRUE(found_position);
    TEST_ASSERT_TRUE(found_velocity);
    TEST_ASSERT_TRUE(found_heading);
    TEST_ASSERT_TRUE(found_baro_alt);
    TEST_ASSERT_TRUE(found_temp);
    TEST_ASSERT_TRUE(found_mag_heading);
    TEST_ASSERT_TRUE(found_timestamp);

    klv_packet_free(packet);
}

/* Test: Position KLV value encoding matches MISB ST 0601 (deg7, mm) */
void test_klv_position_encoding(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(packet);

    /* Find position key (0x0001) */
    uint8_t *ptr = packet->data;
    size_t remaining = packet->len;
    bool found = false;

    while (remaining >= 4) {
        uint16_t key = (ptr[0] << 8) | ptr[1];
        uint16_t length = (ptr[2] << 8) | ptr[3];

        if (key == 0x0001 && length == 12) {
            int32_t lat_deg7 = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
            int32_t lon_deg7 = (ptr[8] << 24) | (ptr[9] << 16) | (ptr[10] << 8) | ptr[11];
            int32_t height_mm = (ptr[12] << 24) | (ptr[13] << 16) | (ptr[14] << 8) | ptr[15];

            /* Verify matches sensor data */
            TEST_ASSERT_EQUAL(sensors.gps.lat_deg7, lat_deg7);
            TEST_ASSERT_EQUAL(sensors.gps.lon_deg7, lon_deg7);
            TEST_ASSERT_EQUAL(sensors.gps.height_mm, height_mm);
            found = true;
            break;
        }

        ptr += 4 + length;
        remaining -= 4 + length;
    }

    TEST_ASSERT_TRUE(found);
    klv_packet_free(packet);
}

/* Test: Velocity KLV value encoding (NED in mm/s) */
void test_klv_velocity_encoding(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(packet);

    uint8_t *ptr = packet->data;
    size_t remaining = packet->len;
    bool found = false;

    while (remaining >= 4) {
        uint16_t key = (ptr[0] << 8) | ptr[1];
        uint16_t length = (ptr[2] << 8) | ptr[3];

        if (key == 0x0002 && length == 12) {
            int32_t vel_n_mm = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
            int32_t vel_e_mm = (ptr[8] << 24) | (ptr[9] << 16) | (ptr[10] << 8) | ptr[11];
            int32_t vel_d_mm = (ptr[12] << 24) | (ptr[13] << 16) | (ptr[14] << 8) | ptr[15];

            TEST_ASSERT_EQUAL(sensors.gps.vel_n_mm, vel_n_mm);
            TEST_ASSERT_EQUAL(sensors.gps.vel_e_mm, vel_e_mm);
            TEST_ASSERT_EQUAL(sensors.gps.vel_d_mm, vel_d_mm);
            found = true;
            break;
        }

        ptr += 4 + length;
        remaining -= 4 + length;
    }

    TEST_ASSERT_TRUE(found);
    klv_packet_free(packet);
}

/* Test: Temperature encoding (int16 * 10) */
void test_klv_temperature_encoding(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(packet);

    uint8_t *ptr = packet->data;
    size_t remaining = packet->len;
    bool found = false;

    while (remaining >= 4) {
        uint16_t key = (ptr[0] << 8) | ptr[1];
        uint16_t length = (ptr[2] << 8) | ptr[3];

        if (key == 0x0103 && length == 2) {
            int16_t temp_x10 = (ptr[4] << 8) | ptr[5];
            int16_t expected = (int16_t)(sensors.aht20.temp * 10.0f);
            TEST_ASSERT_EQUAL(expected, temp_x10);
            found = true;
            break;
        }

        ptr += 4 + length;
        remaining -= 4 + length;
    }

    TEST_ASSERT_TRUE(found);
    klv_packet_free(packet);
}

/* Test: klv_packet_free handles NULL gracefully */
void test_klv_packet_free_null(void)
{
    klv_packet_free(NULL);  /* Should not crash */
    TEST_PASS();
}

/* Test: Frame counter increments timestamp */
void test_klv_timestamp_increments(void)
{
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);

    klv_packet_t *packet1 = klv_builder_build(&sensors, 0);
    klv_packet_t *packet2 = klv_builder_build(&sensors, 1);

    TEST_ASSERT_NOT_NULL(packet1);
    TEST_ASSERT_NOT_NULL(packet2);
    TEST_ASSERT_GREATER_THAN(packet1->timestamp_us, packet2->timestamp_us);

    klv_packet_free(packet1);
    klv_packet_free(packet2);
}