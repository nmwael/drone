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
    sensor_sim_get_data(&data, 0);

    /* Verify GPS data is populated */
    TEST_ASSERT_NOT_EQUAL(0.0, data.gps.lat);
    TEST_ASSERT_NOT_EQUAL(0.0, data.gps.lon);
    TEST_ASSERT_GREATER_THAN(0, data.gps.num_sv);
    TEST_ASSERT_EQUAL(4, data.gps.fix_type);  /* RTK fixed */
}

/* Test: sensor_sim_get_data produces valid IMU data */
void test_sensor_sim_get_data_imu(void)
{
    sensor_data_t data;
    memset(&data, 0, sizeof(sensor_data_t));
    sensor_sim_get_data(&data, 0);

    /* Verify IMU data is populated */
    TEST_ASSERT_GREATER_THAN(0.0f, data.imu.accel_z);  /* Z should be ~1g */
    TEST_ASSERT_NOT_EQUAL(0.0f, data.imu.gyro_x);
}

/* Test: sensor_sim_get_data produces valid sensor aggregation */
void test_sensor_sim_get_data_full(void)
{
    sensor_data_t data;
    memset(&data, 0, sizeof(sensor_data_t));
    sensor_sim_get_data(&data, 0);

    /* Verify all sensor blocks have non-zero data */
    TEST_ASSERT_NOT_EQUAL(0.0, data.gps.lat);
    TEST_ASSERT_NOT_EQUAL(0.0, data.imu.accel_z);
    TEST_ASSERT_NOT_EQUAL(0.0, data.mag.heading);
    TEST_ASSERT_NOT_EQUAL(0.0, data.baro.pressure);
    TEST_ASSERT_NOT_EQUAL(0.0, data.aht20.temp);

    /* Verify AHT20 temperature is reasonable */
    TEST_ASSERT_GREATER_THAN(0.0f, data.aht20.temp);
    TEST_ASSERT_GREATER_THAN(0.0f, data.aht20.humidity);
}