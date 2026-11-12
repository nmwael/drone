/*
 * Sensor Simulator - Implementation
 * Generates simulated sensor data that varies per frame
 */

#include <string.h>
#include <math.h>
#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_SIM";

// Systematic office Aarhus base coordinates
#define BASE_LAT_DEG     56.1735
#define BASE_LON_DEG     10.2040
#define BASE_ALT_M       50.0
#define BASE_HEADING_DEG 45.0

// Simulation parameters
#define MAX_SPEED_M_S    15.0
#define TURN_RATE_DEG_S  5.0

static uint32_t s_frame_counter = 0;

esp_err_t sensor_sim_init(void)
{
    ESP_LOGI(TAG, "Sensor simulator initialized (Systematic Aarhus)");
    s_frame_counter = 0;
    return ESP_OK;
}

void sensor_sim_get_data(sensor_data_t *out, uint32_t frame_num)
{
    if (!out) return;
    memset(out, 0, sizeof(sensor_data_t));
    
    s_frame_counter = frame_num;
    
    // Simulate a circular flight path around Systematic Aarhus
    // Radius: ~500m, completes circle every ~200 frames (~6.6 seconds at 30fps)
    float angle = (frame_num * 2.0f * M_PI) / 200.0f;  // radians
    float radius_m = 500.0f;
    
    // Calculate position offset from base
    float lat_offset_deg = (radius_m * sinf(angle)) / 111320.0f;  // meters to degrees lat
    float lon_offset_deg = (radius_m * cosf(angle)) / (111320.0f * cosf(BASE_LAT_DEG * M_PI / 180.0f));  // meters to degrees lon
    
    float lat = BASE_LAT_DEG + lat_offset_deg;
    float lon = BASE_LON_DEG + lon_offset_deg;
    
    // Altitude varies slightly (simulate terrain following)
    float alt = BASE_ALT_M + 10.0f * sinf(angle * 2.0f);
    
    // Heading follows the circular path (tangent to circle)
    float heading = BASE_HEADING_DEG + 90.0f + (angle * 180.0f / M_PI);
    if (heading > 360.0f) heading -= 360.0f;
    
    // Velocity (tangent to circle, ~15 m/s)
    float speed_m_s = MAX_SPEED_M_S;
    float vel_n = speed_m_s * cosf(angle);
    float vel_e = speed_m_s * sinf(angle);
    float vel_d = 0.0f;
    
    // Fill GPS data
    out->gps.lat = lat;
    out->gps.lon = lon;
    out->gps.alt = alt;
    out->gps.heading = heading;
    out->gps.fix = 4;  // RTK Fixed
    out->gps.sats = 12;
    out->gps.lat_deg7 = (int32_t)(lat * 1e7);
    out->gps.lon_deg7 = (int32_t)(lon * 1e7);
    out->gps.height_mm = (int32_t)(alt * 1000);
    out->gps.heading_deg5 = (int32_t)(heading * 1e5);
    out->gps.p_dop = 100;
    out->gps.fix_type = 4;
    out->gps.num_sv = 12;
    out->gps.vel_n_mm = (int32_t)(vel_n * 1000);
    out->gps.vel_e_mm = (int32_t)(vel_e * 1000);
    out->gps.vel_d_mm = (int32_t)(vel_d * 1000);
    out->gps.tow_ms = 123456789 + frame_num * 33;  // 33ms per frame @ 30fps
    out->gps.week = 2300;
    
    // IMU data - simulating banked turn
    float bank_angle = 10.0f * sinf(angle);  // Bank up to 10 degrees
    out->imu.accel_x = 0.0f;
    out->imu.accel_y = sinf(bank_angle * M_PI / 180.0f) * 0.17f;  // Lateral acceleration
    out->imu.accel_z = 1.0f / cosf(bank_angle * M_PI / 180.0f);   // Vertical acceleration
    out->imu.gyro_x = 0.0f;
    out->imu.gyro_y = 0.0f;
    out->imu.gyro_z = TURN_RATE_DEG_S * M_PI / 180.0f;  // Yaw rate in rad/s
    out->imu.quat_w = 1.0f;
    out->imu.quat_x = 0.0f;
    out->imu.quat_y = 0.0f;
    out->imu.quat_z = 0.0f;
    out->imu.roll = bank_angle;
    out->imu.pitch = 0.0f;
    out->imu.yaw = heading;
    
    // Magnetometer - heading matches GPS heading
    out->mag.heading = heading;
    out->mag.mag_x = 0.2f * cosf(heading * M_PI / 180.0f);
    out->mag.mag_y = 0.2f * sinf(heading * M_PI / 180.0f);
    out->mag.mag_z = 0.5f;
    
    // Barometer - pressure varies with altitude
    out->baro.pressure = 1013.25f - (alt / 8.4f);  // Approx 1 hPa per 8.4m
    out->baro.altitude = alt;
    out->baro.temp = 20.0f - (alt / 100.0f);  // Lapse rate ~1°C per 100m
    
    // AHT20 - cabin temperature/humidity
    out->aht20.temp = 22.0f + 2.0f * sinf(angle);  // Varies 20-24°C
    out->aht20.humidity = 50.0f + 5.0f * cosf(angle);  // Varies 45-55%
}

void sensor_sim_update(void)
{
    // No-op - frame-based simulation
}