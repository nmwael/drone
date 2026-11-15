idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_driver_i2c
        esp_driver_uart
        esp_driver_gpio
        esp_driver_spi
        esp_wifi
        esp_netif
        esp_event
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