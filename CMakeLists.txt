# Top-level CMakeLists.txt for RC Monster Truck Sensor Suite
# Builds all firmware targets for ESP32-S3, ESP32-C3, and RP2040s
# 
# RP2040 Flash Configuration:
#   - RP2040 #1 (ToF Array 1): 16MB (128 Mbit) Flash - Front, Rear, Left, Right
#   - RP2040 #2 (ToF Array 2): 4MB (32 Mbit) Flash  - Front-Left, Left, Rear-Left, Aux
#   - RP2040 #3 (ToF Array 3): 4MB (32 Mbit) Flash  - Front-Right, Right, Rear-Right, Aux
#   All have 264KB SRAM, dual-core Cortex-M0+, PIO

cmake_minimum_required(VERSION 3.16)
project(drone_rc_truck LANGUAGES C CXX ASM)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Common compile options
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror=implicit-function-declaration
    -Werror=pointer-sign
    -std=gnu11
)

# Include subdirectories for each MCU target
add_subdirectory(firmware/esp32-s3-vision)
add_subdirectory(firmware/esp32-c3-hub)
add_subdirectory(firmware/rp2040-tof-1)
add_subdirectory(firmware/rp2040-tof-2)
add_subdirectory(firmware/rp2040-tof-3)

# Custom targets for flashing
add_custom_target(flash-s3
    COMMAND idf.py -B build/esp32-s3-vision flash
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/esp32-s3-vision
    COMMENT "Flashing ESP32-S3 Vision"
)

add_custom_target(flash-c3
    COMMAND idf.py -B build/esp32-c3-hub flash
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/esp32-c3-hub
    COMMENT "Flashing ESP32-C3 Hub"
)

add_custom_target(flash-tof1
    COMMAND picotool load build/rp2040-tof-1/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-1
    COMMENT "Flashing RP2040 ToF Array #1"
)

add_custom_target(flash-tof2
    COMMAND picotool load build/rp2040-tof-2/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-2
    COMMENT "Flashing RP2040 ToF Array #2"
)

add_custom_target(flash-tof3
    COMMAND picotool load build/rp2040-tof-3/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-3
    COMMENT "Flashing RP2040 ToF Array #3"
)

# Aggregate flash target
add_custom_target(flash-all
    DEPENDS flash-s3 flash-c3 flash-tof1 flash-tof2 flash-tof3
    COMMENT "Flashing all targets"
)

# Monitor targets
add_custom_target(monitor-s3
    COMMAND idf.py -B build/esp32-s3-vision monitor
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/esp32-s3-vision
)

add_custom_target(monitor-c3
    COMMAND idf.py -B build/esp32-c3-hub monitor
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/esp32-c3-hub
)

# Clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/esp32-s3-vision/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/esp32-c3-hub/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-1/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-2/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-tof-3/build
    COMMENT "Cleaning all build directories"
)