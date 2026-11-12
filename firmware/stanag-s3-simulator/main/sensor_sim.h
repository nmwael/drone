/*
 * Sensor Simulator - Header File
 */

#ifndef SENSOR_SIM_H
#define SENSOR_SIM_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

esp_err_t sensor_sim_init(void);
void sensor_sim_get_data(sensor_data_t *out, uint32_t frame_num);
void sensor_sim_update(void);
esp_err_t sensor_sim_deinit(void);

#endif // SENSOR_SIM_H