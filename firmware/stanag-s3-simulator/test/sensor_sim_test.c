/*
 * Sensor Simulator Unit Tests
 *
 * Tests for the sensor simulator stub implementations.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "sensor_sim.h"
#include "stanag_types.h"

/* Test setup/teardown */
void setUp(void) {
    /* Initialize before each test */
    sensor_sim_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: sensor_sim_init returns ESP_OK */
void test_sensor_sim_init(void)
{
    esp_err_t ret = sensor_sim_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* Test: sensor_sim_get_data produces valid GPS data */
void test_sensor_sim_get_data_gps(void)
{
    sensor_data_t data;
    memset(&data, 0, sizeof(sensor_data_t));
    sensor_sim_get_data(&data);

    /* Verify GPS data is populated */
    TEST_ASSERT_NOT_NULL(data.gps.lat);
    TEST_ASSERT_NOT_NULL(data.gps.lon);
    TEST_ASSERT_GREATER_THAN(0, data.gps.num_sv);
    TEST_ASSERT_EQUAL(4, data.gps.fix_type);  /* RTK fixed */
}

/* Test: sensor_sim_get_data produces valid IMU data */
void test_sensor_sim_get_data_imu(void)
{
    sensor_data_t data;
    memset(&data, 0, sizeof(sensor_data_t));
    sensor_sim_get_data(&data);

    /* Verify IMU data is populated */
    TEST_ASSERT_NOT_NULL(&data.imu);
    TEST_ASSERT_GREATER_THAN(0, data.imu.accel_z);  /* Z should be ~1g */
    TEST_ASSERT_NOT_NULL(&data.imu.gyro_x);
}

/* Test: sensor_sim_get_data produces valid sensor aggregation */
void test_sensor_sim_get_data_full(void)
{
    sensor_data_t data;
    memset(&data, 0, sizeof(sensor_data_t));
    sensor_sim_get_data(&data);

    /* Verify all sensor blocks are initialized */
    TEST_ASSERT_NOT_NULL(&data.gps);
    TEST_ASSERT_NOT_NULL(&data.imu);
    TEST_ASSERT_NOT_NULL(&data.mag);
    TEST_ASSERT_NOT_NULL(&data.baro);
    TEST_ASSERT_NOT_NULL(&data.aht20);

    /* Verify AHT20 temperature is reasonable */
    TEST_ASSERT_GREATER_THAN(0, data.aht20.temp);
    TEST_ASSERT_GREATER_THAN(0, data.aht20.humidity);
}