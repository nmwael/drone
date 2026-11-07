# RP2040 Rear ToF + Aux Firmware - CMakeLists.txt
# VL53L5CX x2 (rear, right) + UART command

cmake_minimum_required(VERSION 3.13)

if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to Pico SDK")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040-rear-tof C CXX ASM)

pico_sdk_init()

set(PICO_BOARD pico)

pico_enable_stdio_uart(rp2040-rear-tof 1)
pico_enable_stdio_usb(rp2040-rear-tof 0)

add_executable(rp2040-rear-tof
    src/vl53l5cx.c
    src/uart_cmd.c
    pio/i2c_pio.pio.c
)

target_link_libraries(rp2040-rear-tof PRIVATE
    pico_stdlib
    hardware_pio
    hardware_uart
    hardware_i2c
    hardware_irq
    hardware_dma
)

pico_add_extra_outputs(rp2040-rear-tof)

target_compile_options(rp2040-rear-tof PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -std=gnu11
    -O2
)