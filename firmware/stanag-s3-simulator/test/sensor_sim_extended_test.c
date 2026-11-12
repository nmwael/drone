/*
 * Sensor Simulator Extended Unit Tests
 *
 * Additional tests for sensor simulation.
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "unity.h"
#include "sensor_sim.h"
#include "stanag_types.h"

/* Test setup/teardown */
void setUp(void) {
    sensor_sim_init();
}

void tearDown(void) {
    /* Clean up after each test */
}

/* Test: GPS position follows circular path */
void test_sensor_sim_gps_circular_path(void)
{
    sensor_data_t data0, data50, data100, data150;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data50, 50);
    sensor_sim_get_data(&data100, 100);
    sensor_sim_get_data(&data150, 150);

    /* All should have valid RTK fix */
    TEST_ASSERT_EQUAL(4, data0.gps.fix_type);
    TEST_ASSERT_EQUAL(4, data50.gps.fix_type);
    TEST_ASSERT_EQUAL(4, data100.gps.fix_type);
    TEST_ASSERT_EQUAL(4, data150.gps.fix_type);

    /* Positions should be different (circular motion) */
    TEST_ASSERT_NOT_EQUAL(data0.gps.lat_deg7, data50.gps.lat_deg7);
    TEST_ASSERT_NOT_EQUAL(data0.gps.lon_deg7, data50.gps.lon_deg7);
    TEST_ASSERT_NOT_EQUAL(data50.gps.lat_deg7, data100.gps.lat_deg7);
    TEST_ASSERT_NOT_EQUAL(data50.gps.lon_deg7, data100.gps.lon_deg7);

    /* After 200 frames should be back to start (full circle) */
    sensor_data_t data200;
    sensor_sim_get_data(&data200, 200);

    /* Allow small numerical differences */
    TEST_ASSERT_INT32_WITHIN(1000, data0.gps.lat_deg7, data200.gps.lat_deg7);
    TEST_ASSERT_INT32_WITHIN(1000, data0.gps.lon_deg7, data200.gps.lon_deg7);
}

/* Test: GPS altitude varies with terrain following */
void test_sensor_sim_gps_altitude_variation(void)
{
    sensor_data_t data0, data25, data50, data75;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data25, 25);
    sensor_sim_get_data(&data50, 50);
    sensor_sim_get_data(&data75, 75);

    /* Altitude should vary (sinusoidal with 2x frequency) */
    TEST_ASSERT_NOT_EQUAL(data0.gps.height_mm, data25.gps.height_mm);
    TEST_ASSERT_NOT_EQUAL(data25.gps.height_mm, data50.gps.height_mm);

    /* Should be within reasonable range (40-60m) */
    TEST_ASSERT_GREATER_THAN(40000, data0.gps.height_mm);
    TEST_ASSERT_LESS_THAN(60000, data0.gps.height_mm);
}

/* Test: GPS heading follows tangent to circle */
void test_sensor_sim_gps_heading(void)
{
    sensor_data_t data0, data50;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data50, 50);

    /* Heading should change as we move around circle */
    TEST_ASSERT_NOT_EQUAL(data0.gps.heading_deg5, data50.gps.heading_deg5);

    /* Heading should be 0-360 degrees */
    TEST_ASSERT_GREATER_OR_EQUAL(0, data0.gps.heading_deg5);
    TEST_ASSERT_LESS_OR_EQUAL(36000000, data0.gps.heading_deg5);
}

/* Test: GPS velocity is tangent to circular path */
void test_sensor_sim_gps_velocity(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    /* Speed should be ~15 m/s = 15000 mm/s */
    int32_t speed_mm = data.gps.speed_mm;
    TEST_ASSERT_INT32_WITHIN(1000, 15000, speed_mm);

    /* Velocity components should be reasonable */
    TEST_ASSERT_NOT_EQUAL(0, data.gps.vel_n_mm);
    TEST_ASSERT_NOT_EQUAL(0, data.gps.vel_e_mm);
    TEST_ASSERT_EQUAL(0, data.gps.vel_d_mm);  /* Level flight */
}

/* Test: IMU shows banked turn */
void test_sensor_sim_imu_banked_turn(void)
{
    sensor_data_t data0, data50;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data50, 50);

    /* Roll should vary (bank angle up to 10 degrees) */
    TEST_ASSERT_NOT_EQUAL(data0.imu.roll, data50.imu.roll);
    TEST_ASSERT_FLOAT_WITHIN(11.0f, 0.0f, data0.imu.roll);  /* ±10 degrees */
    TEST_ASSERT_FLOAT_WITHIN(11.0f, 0.0f, data50.imu.roll);

    /* Pitch should be near 0 (level flight) */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, data0.imu.pitch);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, data50.imu.pitch);

    /* Yaw should match heading */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, data0.gps.heading, data0.imu.yaw);
}

/* Test: IMU accelerometer shows 1g in Z with bank correction */
void test_sensor_sim_imu_accel_z(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    /* Z accel should be >= 1g (1/cos(bank)) */
    TEST_ASSERT_GREATER_OR_EQUAL(1.0f, data.imu.accel_z);
    TEST_ASSERT_LESS_THAN(1.02f, data.imu.accel_z);  /* cos(10°) ≈ 0.985, 1/0.985 ≈ 1.015 */
}

/* Test: IMU gyro Z shows turn rate */
void test_sensor_sim_imu_gyro_z(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    /* Gyro Z should be turn rate in rad/s (5 deg/s = 0.087 rad/s) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f * M_PI / 180.0f, data.imu.gyro_z);
}

/* Test: Magnetometer heading matches GPS heading */
void test_sensor_sim_mag_heading(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, data.mag.heading, data.gps.heading);
}

/* Test: Barometer pressure decreases with altitude */
void test_sensor_sim_baro_pressure_altitude(void)
{
    sensor_data_t data_low, data_high;

    sensor_sim_get_data(&data_low, 0);   /* alt ~50m */
    sensor_sim_get_data(&data_high, 25); /* alt ~60m (sin(pi/2) = 1) */

    /* Higher altitude = lower pressure */
    TEST_ASSERT_GREATER_THAN(data_high.baro.pressure, data_low.baro.pressure);

    /* Pressure should be around 1013 hPa at sea level, less at altitude */
    TEST_ASSERT_GREATER_THAN(1000.0f, data_low.baro.pressure);
    TEST_ASSERT_LESS_THAN(1020.0f, data_low.baro.pressure);
}

/* Test: Barometer temperature follows lapse rate */
void test_sensor_sim_baro_temp_lapse_rate(void)
{
    sensor_data_t data_low, data_high;

    sensor_sim_get_data(&data_low, 0);   /* alt ~50m */
    sensor_sim_get_data(&data_high, 25); /* alt ~60m */

    /* Higher altitude = lower temperature (lapse rate ~1°C/100m) */
    TEST_ASSERT_GREATER_THAN(data_high.baro.temp, data_low.baro.temp);
}

/* Test: AHT20 cabin temperature varies 20-24°C */
void test_sensor_sim_aht20_temp_range(void)
{
    float min_temp = 100.0f, max_temp = -100.0f;

    for (int i = 0; i < 200; i++) {
        sensor_data_t data;
        sensor_sim_get_data(&data, i);
        if (data.aht20.temp < min_temp) min_temp = data.aht20.temp;
        if (data.aht20.temp > max_temp) max_temp = data.aht20.temp;
    }

    TEST_ASSERT_FLOAT_WITHIN(2.0f, 22.0f, min_temp);  /* ~20°C min */
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 24.0f, max_temp);  /* ~24°C max */
}

/* Test: AHT20 humidity varies 45-55% */
void test_sensor_sim_aht20_humidity_range(void)
{
    float min_hum = 100.0f, max_hum = 0.0f;

    for (int i = 0; i < 200; i++) {
        sensor_data_t data;
        sensor_sim_get_data(&data, i);
        if (data.aht20.humidity < min_hum) min_hum = data.aht20.humidity;
        if (data.aht20.humidity > max_hum) max_hum = data.aht20.humidity;
    }

    TEST_ASSERT_FLOAT_WITHIN(5.0f, 50.0f, min_hum);  /* ~45% min */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 50.0f, max_hum);  /* ~55% max */
}

/* Test: GPS time increments by 33ms per frame */
void test_sensor_sim_gps_time_increment(void)
{
    sensor_data_t data0, data1;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data1, 1);

    TEST_ASSERT_EQUAL(data0.gps.tow_ms + 33, data1.gps.tow_ms);
}

/* Test: GPS week number is constant */
void test_sensor_sim_gps_week(void)
{
    sensor_data_t data0, data100;

    sensor_sim_get_data(&data0, 0);
    sensor_sim_get_data(&data100, 100);

    TEST_ASSERT_EQUAL(data0.gps.week, data100.gps.week);
    TEST_ASSERT_EQUAL(2300, data0.gps.week);
}

/* Test: Satellite count is reasonable */
void test_sensor_sim_gps_satellites(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    TEST_ASSERT_EQUAL(12, data.gps.num_sv);
    TEST_ASSERT_EQUAL(12, data.gps.sats);
}

/* Test: PDOP is reasonable for RTK */
void test_sensor_sim_gps_pdop(void)
{
    sensor_data_t data;
    sensor_sim_get_data(&data, 0);

    /* PDOP * 0.01 = 100 means PDOP = 1.0 (excellent for RTK) */
    TEST_ASSERT_EQUAL(100, data.gps.p_dop);
}