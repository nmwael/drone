/*
 * Common data structures for STANAG S3 Simulator
 */

#ifndef STANAG_TYPES_H
#define STANAG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* --- JPEG Frame --- */
typedef struct {
    uint8_t *data;
    size_t len;
    int width;
    int height;
    int index;          // 0-4
} jpeg_frame_t;

/* --- YUV420 Frame (I420 planar) --- */
typedef struct {
    uint8_t *y;         // Width x Height
    uint8_t *u;         // Width/2 x Height/2
    uint8_t *v;         // Width/2 x Height/2
    int width;
    int height;
    int stride_y;
    int stride_uv;
    uint64_t timestamp_us;
} yuv420_frame_t;

/* --- H.264 NAL Unit --- */
typedef struct {
    uint8_t *data;
    size_t len;
    int type;           // NAL type (IDR, SPS, PPS, slice)
    uint64_t timestamp_us;
    bool keyframe;
} h264_nalu_t;

/* --- KLV Packet --- */
typedef struct {
    uint8_t *data;
    size_t len;
    uint64_t timestamp_us;
} klv_packet_t;

/* --- MPEG-TS Packet --- */
typedef struct {
    uint8_t *data;      // 188 bytes each
    size_t count;       // Number of 188-byte packets
    size_t total_len;   // count * 188
    uint64_t timestamp_us;
} ts_packet_t;

/* --- GPS Data (simulated u-blox ZED-F9P) --- */
typedef struct {
    uint32_t week;          // GPS week
    uint32_t tow_ms;        // Time of week (ms)
    int32_t lat_deg7;       // Latitude * 1e7 (deg)
    int32_t lon_deg7;       // Longitude * 1e7 (deg)
    int32_t height_mm;      // Ellipsoid height (mm)
    int32_t h_msl_mm;       // MSL height (mm)
    int32_t vel_n_mm;       // Velocity North (mm/s)
    int32_t vel_e_mm;       // Velocity East (mm/s)
    int32_t vel_d_mm;       // Velocity Down (mm/s)
    uint32_t speed_mm;      // Speed (mm/s)
    int32_t heading_deg5;   // Heading * 1e5 (deg)
    uint16_t p_dop;         // PDOP * 0.01
    uint8_t fix_type;       // 0=none, 2=2D, 3=3D, 4=RTK fixed
    uint8_t num_sv;         // Satellites used
    double lat;             // Latitude (deg)
    double lon;             // Longitude (deg)
    double alt;             // Altitude (m)
    double heading;         // Heading (deg)
    int fix;                // 0=none, 1=GPS, 2=DGPS, 4=RTK fixed
    int sats;               // Satellites tracked
} gps_data_t;

/* --- IMU Data (simulated ICM-20948) --- */
typedef struct {
    float accel_x;      // g
    float accel_y;
    float accel_z;
    float gyro_x;       // deg/s
    float gyro_y;
    float gyro_z;
    float quat_w;
    float quat_x;
    float quat_y;
    float quat_z;
    float roll;         // deg
    float pitch;        // deg
    float yaw;          // deg
} imu_data_t;

/* --- Magnetometer Data (simulated QMC5883L) --- */
typedef struct {
    float mag_x;        // uT
    float mag_y;
    float mag_z;
    float heading;      // deg (0-360)
} mag_data_t;

/* --- Barometer Data (simulated BMP280) --- */
typedef struct {
    float pressure;     // hPa
    float altitude;     // m
    float temp;         // degC
} baro_data_t;

/* --- AHT20 Data --- */
typedef struct {
    float temp;         // degC
    float humidity;     // %RH
} aht20_data_t;

/* --- Aggregated Sensor Data --- */
typedef struct {
    gps_data_t gps;
    imu_data_t imu;
    mag_data_t mag;
    baro_data_t baro;
    aht20_data_t aht20;
} sensor_data_t;

/* --- Function Prototypes --- */

/* JPEG Loader */
jpeg_frame_t* jpeg_loader_get_next(void);
yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame);
esp_err_t jpeg_loader_init(void);

/* Sensor Simulators */
esp_err_t sensor_sim_init(void);
void sensor_sim_get_data(sensor_data_t *out);
void sensor_sim_update(void);

/* H.264 Encoder */
esp_err_t h264_encoder_init(void);
h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv);

/* KLV Builder */
esp_err_t klv_builder_init(void);
klv_packet_t* klv_builder_build(sensor_data_t *sensors, uint32_t frame_num);

/* TS Muxer */
esp_err_t ts_muxer_init(void);
ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv);

/* Network TX */
esp_err_t network_tx_init(void);
void network_tx_send(ts_packet_t *ts);

/* UART Telemetry */
esp_err_t uart_telemetry_init(void);
void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num);

#endif // STANAG_TYPES_H