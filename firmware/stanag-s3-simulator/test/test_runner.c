/*
 * Test Runner - Main entry point for all unit tests
 */

#include <stdio.h>
#include "unity.h"
#include "esp_log.h"

extern void test_klv_builder_init(void);
extern void test_klv_builder_build_valid_data(void);
extern void test_klv_builder_build_null_sensors(void);
extern void test_klv_packet_contains_expected_keys(void);
extern void test_klv_position_encoding(void);
extern void test_klv_velocity_encoding(void);
extern void test_klv_temperature_encoding(void);
extern void test_klv_packet_free_null(void);
extern void test_klv_timestamp_increments(void);

extern void test_ts_muxer_init(void);
extern void test_ts_muxer_mux_valid_nalu(void);
extern void test_ts_muxer_mux_null_nalu(void);
extern void test_ts_packet_sync_byte(void);
extern void test_ts_video_pid(void);
extern void test_ts_klv_pid(void);
extern void test_ts_continuity_counter(void);
extern void test_ts_pusi_set(void);
extern void test_ts_adaptation_field(void);
extern void test_ts_packet_size(void);
extern void test_ts_null_packet(void);
extern void test_ts_packet_free_null(void);

extern void test_h264_encoder_init(void);
extern void test_h264_encoder_encode_valid_yuv(void);
extern void test_h264_encoder_encode_null_yuv(void);
extern void test_h264_nalu_start_code(void);
extern void test_h264_nalu_idr_type(void);
extern void test_h264_timestamp_propagation(void);
extern void test_h264_keyframe_flag(void);
extern void test_h264_multiple_encodes(void);
extern void test_h264_nalu_type_field(void);

extern void test_jpeg_loader_init(void);
extern void test_jpeg_loader_get_next(void);
extern void test_jpeg_loader_frame_cycling(void);
extern void test_jpeg_loader_decode(void);
extern void test_jpeg_loader_decode_null_frame(void);
extern void test_jpeg_loader_decode_empty_frame(void);
extern void test_yuv_buffer_layout(void);
extern void test_yuv_test_pattern(void);
extern void test_jpeg_loader_free_yuv_null(void);
extern void test_jpeg_loader_static_buffer(void);

extern void test_sensor_sim_init(void);
extern void test_sensor_sim_get_data_gps(void);
extern void test_sensor_sim_get_data_imu(void);
extern void test_sensor_sim_get_data_full(void);
extern void test_sensor_sim_gps_circular_path(void);
extern void test_sensor_sim_gps_altitude_variation(void);
extern void test_sensor_sim_gps_heading(void);
extern void test_sensor_sim_gps_velocity(void);
extern void test_sensor_sim_imu_banked_turn(void);
extern void test_sensor_sim_imu_accel_z(void);
extern void test_sensor_sim_imu_gyro_z(void);
extern void test_sensor_sim_mag_heading(void);
extern void test_sensor_sim_baro_pressure_altitude(void);
extern void test_sensor_sim_baro_temp_lapse_rate(void);
extern void test_sensor_sim_aht20_temp_range(void);
extern void test_sensor_sim_aht20_humidity_range(void);
extern void test_sensor_sim_gps_time_increment(void);
extern void test_sensor_sim_gps_week(void);
extern void test_sensor_sim_gps_satellites(void);
extern void test_sensor_sim_gps_pdop(void);

extern void test_network_tx_init(void);
extern void test_network_tx_set_udp_dest(void);
extern void test_network_tx_set_espnow_peer(void);
extern void test_network_tx_send_valid_ts(void);
extern void test_network_tx_send_null_ts(void);
extern void test_network_tx_send_empty_ts(void);
extern void test_network_tx_deinit(void);

extern void test_uart_telemetry_init(void);
extern void test_uart_telemetry_send_valid(void);
extern void test_uart_telemetry_send_null_sensors(void);
extern void test_uart_telemetry_send_null_nalu(void);
extern void test_uart_telemetry_deinit(void);

void app_main(void)
{
    ESP_LOGI("TEST_RUNNER", "Starting unit tests...");

    UNITY_BEGIN();

    /* KLV Builder Tests */
    RUN_TEST(test_klv_builder_init);
    RUN_TEST(test_klv_builder_build_valid_data);
    RUN_TEST(test_klv_builder_build_null_sensors);
    RUN_TEST(test_klv_packet_contains_expected_keys);
    RUN_TEST(test_klv_position_encoding);
    RUN_TEST(test_klv_velocity_encoding);
    RUN_TEST(test_klv_temperature_encoding);
    RUN_TEST(test_klv_packet_free_null);
    RUN_TEST(test_klv_timestamp_increments);

    /* TS Muxer Tests */
    RUN_TEST(test_ts_muxer_init);
    RUN_TEST(test_ts_muxer_mux_valid_nalu);
    RUN_TEST(test_ts_muxer_mux_null_nalu);
    RUN_TEST(test_ts_packet_sync_byte);
    RUN_TEST(test_ts_video_pid);
    RUN_TEST(test_ts_klv_pid);
    RUN_TEST(test_ts_continuity_counter);
    RUN_TEST(test_ts_pusi_set);
    RUN_TEST(test_ts_adaptation_field);
    RUN_TEST(test_ts_packet_size);
    RUN_TEST(test_ts_null_packet);
    RUN_TEST(test_ts_packet_free_null);

    /* H.264 Encoder Tests */
    RUN_TEST(test_h264_encoder_init);
    RUN_TEST(test_h264_encoder_encode_valid_yuv);
    RUN_TEST(test_h264_encoder_encode_null_yuv);
    RUN_TEST(test_h264_nalu_start_code);
    RUN_TEST(test_h264_nalu_idr_type);
    RUN_TEST(test_h264_timestamp_propagation);
    RUN_TEST(test_h264_keyframe_flag);
    RUN_TEST(test_h264_multiple_encodes);
    RUN_TEST(test_h264_nalu_type_field);

    /* JPEG Loader Tests */
    RUN_TEST(test_jpeg_loader_init);
    RUN_TEST(test_jpeg_loader_get_next);
    RUN_TEST(test_jpeg_loader_frame_cycling);
    RUN_TEST(test_jpeg_loader_decode);
    RUN_TEST(test_jpeg_loader_decode_null_frame);
    RUN_TEST(test_jpeg_loader_decode_empty_frame);
    RUN_TEST(test_yuv_buffer_layout);
    RUN_TEST(test_yuv_test_pattern);
    RUN_TEST(test_jpeg_loader_free_yuv_null);
    RUN_TEST(test_jpeg_loader_static_buffer);

    /* Sensor Simulator Tests */
    RUN_TEST(test_sensor_sim_init);
    RUN_TEST(test_sensor_sim_get_data_gps);
    RUN_TEST(test_sensor_sim_get_data_imu);
    RUN_TEST(test_sensor_sim_get_data_full);
    RUN_TEST(test_sensor_sim_gps_circular_path);
    RUN_TEST(test_sensor_sim_gps_altitude_variation);
    RUN_TEST(test_sensor_sim_gps_heading);
    RUN_TEST(test_sensor_sim_gps_velocity);
    RUN_TEST(test_sensor_sim_imu_banked_turn);
    RUN_TEST(test_sensor_sim_imu_accel_z);
    RUN_TEST(test_sensor_sim_imu_gyro_z);
    RUN_TEST(test_sensor_sim_mag_heading);
    RUN_TEST(test_sensor_sim_baro_pressure_altitude);
    RUN_TEST(test_sensor_sim_baro_temp_lapse_rate);
    RUN_TEST(test_sensor_sim_aht20_temp_range);
    RUN_TEST(test_sensor_sim_aht20_humidity_range);
    RUN_TEST(test_sensor_sim_gps_time_increment);
    RUN_TEST(test_sensor_sim_gps_week);
    RUN_TEST(test_sensor_sim_gps_satellites);
    RUN_TEST(test_sensor_sim_gps_pdop);

    /* Network TX Tests */
    RUN_TEST(test_network_tx_init);
    RUN_TEST(test_network_tx_set_udp_dest);
    RUN_TEST(test_network_tx_set_espnow_peer);
    RUN_TEST(test_network_tx_send_valid_ts);
    RUN_TEST(test_network_tx_send_null_ts);
    RUN_TEST(test_network_tx_send_empty_ts);
    RUN_TEST(test_network_tx_deinit);

    /* UART Telemetry Tests */
    RUN_TEST(test_uart_telemetry_init);
    RUN_TEST(test_uart_telemetry_send_valid);
    RUN_TEST(test_uart_telemetry_send_null_sensors);
    RUN_TEST(test_uart_telemetry_send_null_nalu);
    RUN_TEST(test_uart_telemetry_deinit);

    UNITY_END();

    ESP_LOGI("TEST_RUNNER", "All tests completed");
}