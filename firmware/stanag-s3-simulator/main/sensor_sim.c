/*
 * Sensor Simulator - Stub Implementation
 */

#include <string.h>
#include "stanag_types.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_SIM";

esp_err_t sensor_sim_init(void)
{
    ESP_LOGI(TAG, "Sensor simulator initialized (stub)");
    return ESP_OK;
}

void sensor_sim_get_data(sensor_data_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(sensor_data_t));
    
    // Fill with dummy data
    out->gps.lat = 55.6761;   // Copenhagen
    out->gps.lon = 12.5683;
    out->gps.alt = 10.0;
    out->gps.heading = 45.0;
    out->gps.fix = 4;
    out->gps.sats = 12;
    out->gps.lat_deg7 = 556761000;
    out->gps.lon_deg7 = 125683000;
    out->gps.height_mm = 10000;
    out->gps.heading_deg5 = 4500000;
    out->gps.p_dop = 100;
    out->gps.fix_type = 4;
    out->gps.num_sv = 12;
    
    out->imu.accel_x = 0.0f;
    out->imu.accel_y = 0.0f;
    out->imu.accel_z = 1.0f;
    out->imu.gyro_x = 0.0f;
    out->imu.gyro_y = 0.0f;
    out->imu.gyro_z = 0.0f;
    out->imu.quat_w = 1.0f;
    out->imu.quat_x = 0.0f;
    out->imu.quat_y = 0.0f;
    out->imu.quat_z = 0.0f;
    out->imu.roll = 0.0f;
    out->imu.pitch = 0.0f;
    out->imu.yaw = 45.0f;
    
    out->mag.heading = 45.0f;
    out->baro.pressure = 1013.25f;
    out->baro.altitude = 10.0f;
    out->baro.temp = 20.0f;
    out->aht20.temp = 20.0f;
    out->aht20.humidity = 50.0f;
}

void sensor_sim_update(void)
{
    // No-op
}