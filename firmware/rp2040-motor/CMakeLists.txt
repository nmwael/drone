# RP2040 Motor Control Firmware - CMakeLists.txt
# PID velocity control + encoder feedback + RC PWM input + UART command

cmake_minimum_required(VERSION 3.13)

# Pico SDK path (set via environment or CMake cache)
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} CACHE PATH "Path to Pico SDK")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rp2040-motor C CXX ASM)

pico_sdk_init()

# Board configuration
set(PICO_BOARD pico)  # Change if using custom board

# Enable required features
pico_enable_stdio_uart(rp2040-motor 1)
pico_enable_stdio_usb(rp2040-motor 0)

add_executable(rp2040-motor
    src/motor_pid.c
    src/encoder.c
    src/uart_cmd.c
    src/rc_pwm_input.c
    pio/motor_pwm.pio.c
    pio/encoder_quadrature.pio.c
)

# Link Pico SDK libraries
target_link_libraries(rp2040-motor PRIVATE
    pico_stdlib
    hardware_pio
    hardware_pwm
    hardware_uart
    hardware_irq
    hardware_dma
    hardware_adc
)

# Generate UF2 for easy flashing
pico_add_extra_outputs(rp2040-motor)

# Compiler options
target_compile_options(rp2040-motor PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -std=gnu11
    -O2
)