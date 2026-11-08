cmake_minimum_required(VERSION 3.16)

# ESP-IDF configuration
set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

project(stanag-s3-simulator)

# Required ESP-IDF components
idf_component_register(
    SRCS
        "main/main.c"
        "main/jpeg_loader.c"
        "main/sensor_sim.c"
        "main/h264_encoder.c"
        "main/klv_builder.c"
        "main/ts_muxer.c"
        "main/network_tx.c"
        "main/uart_telemetry.c"
    INCLUDE_DIRS
        "main"
        "components"
    REQUIRES
        esp_driver_i2c
        esp_driver_uart
        esp_driver_gpio
        esp_jpeg
        esp_video
        esp_codec
        esp_wifi
        esp_netif
        esp_event
        esp_now
        nvs_flash
        esp_timer
        freertos
        esp_psram
        mbedtls
        cxx
        log
        spiffs
    PRIV_REQUIRES
        esp_dl
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
)

target_link_libraries(${COMPONENT_LIB} PRIVATE
    esp_dl
)