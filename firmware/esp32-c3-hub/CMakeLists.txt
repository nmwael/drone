# ESP32-C3 Sensor Hub Firmware - CMakeLists.txt
# GPS + Mag + IMU + Baro + Temp/Humidity + Sensor Fusion EKF

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

project(esp32-c3-hub)

idf_component_register(
    SRCS
        "main/gps_gnss.c"
        "main/magnetometer.c"
        "main/imu_baro.c"
        "main/aht20.c"
        "main/bmp280.c"
        "main/sensor_fusion_ekf.c"
        "main/comms_espnow.c"
        "main/comms_uart_rp2040.c"
        "main/telemetry_provider.c"
    INCLUDE_DIRS
        "main"
        "components"
    REQUIRES
        esp_driver_i2c
        esp_driver_uart
        esp_driver_gpio
        esp_driver_spi
        esp_wifi
        esp_netif
        esp_event
        esp_now
        nvs_flash
        esp_timer
        freertos
        mbedtls
        log
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
)