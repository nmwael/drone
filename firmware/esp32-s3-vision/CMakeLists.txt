# ESP32-S3 Vision Firmware - CMakeLists.txt
# STANAG 4609 video encoding + thermal/IR vision + TFLite inference

cmake_minimum_required(VERSION 3.16)

# ESP-IDF configuration
set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

# Project name
project(esp32-s3-vision)

# Required ESP-IDF components
idf_component_register(
    SRCS
        "main/thermal_camera.c"
        "main/ir_camera.c"
        "main/vision_inference.c"
        "main/comms_uart.c"
        "main/h264_encoder.c"
        "main/klv_builder.c"
        "main/ts_muxer.c"
        "main/wifi_stream.c"
    INCLUDE_DIRS
        "main"
        "components"
    REQUIRES
        esp_driver_i2c
        esp_driver_spi
        esp_driver_uart
        esp_driver_gpio
        esp_camera
        esp_video
        esp_codec
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        esp_timer
        freertos
        esp_psram
        mbedtls
        cxx
        log
    PRIV_REQUIRES
        esp_dl
        tflite_micro
)

# Compiler options for this component
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
)

# Link ESP-DL and TFLite Micro
target_link_libraries(${COMPONENT_LIB} PRIVATE
    esp_dl
    tflite_micro
)