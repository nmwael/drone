# RP2040 #1 ToF Firmware - CMakeLists.txt
# 4x VL53L5CX ToF Array (Front, Rear, Left, Right) - 16MB Flash

cmake_minimum_required(VERSION 3.13)

if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to Pico SDK")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040-tof-1 C CXX ASM)

pico_sdk_init()

set(PICO_BOARD pico)

pico_enable_stdio_uart(rp2040-tof-1 1)
pico_enable_stdio_usb(rp2040-tof-1 0)

add_executable(rp2040-tof-1
    src/vl53l5cx_4x.c
    src/uart_cmd.c
    src/i2c_mux.c
    pio/i2c_pio.pio.c
)

target_link_libraries(rp2040-tof-1 PRIVATE
    pico_stdlib
    hardware_pio
    hardware_uart
    hardware_i2c
    hardware_irq
    hardware_dma
)

pico_add_extra_outputs(rp2040-tof-1)

target_compile_options(rp2040-tof-1 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -std=gnu11
    -O2
)