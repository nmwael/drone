/*
 * Sensor Simulator - Header File
 */

#ifndef SENSOR_SIM_H
#define SENSOR_SIM_H

#include "stanag_types.h"
#include "esp_err.h"

/* Function Prototypes */

/* JPEG Loader */
esp_err_t jpeg_loader_init(void);
jpeg_frame_t* jpeg_loader_get_next(void);
yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame);
void jpeg_loader_free_yuv(yuv420_frame_t *yuv);

/* Sensor Simulators */
esp_err_t sensor_sim_init(void);
void sensor_sim_get_data(sensor_data_t *out);
void sensor_sim_update(void);

#endif // SENSOR_SIM_H