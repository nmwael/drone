# Top-level CMakeLists.txt for RC Monster Truck Sensor Suite
# Builds all firmware targets for ESP32-S3, ESP32-C3, and RP2040s

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
add_subdirectory(firmware/rp2040-motor)
add_subdirectory(firmware/rp2040-steering-tof)
add_subdirectory(firmware/rp2040-rear-tof)

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

add_custom_target(flash-motor
    COMMAND picotool load build/rp2040-motor/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-motor
    COMMENT "Flashing RP2040 Motor"
)

add_custom_target(flash-steering
    COMMAND picotool load build/rp2040-steering-tof/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-steering-tof
    COMMENT "Flashing RP2040 Steering+ToF"
)

add_custom_target(flash-rear
    COMMAND picotool load build/rp2040-rear-tof/firmware.uf2 -f
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rp2040-rear-tof
    COMMENT "Flashing RP2040 Rear ToF"
)

# Aggregate flash target
add_custom_target(flash-all
    DEPENDS flash-s3 flash-c3 flash-motor flash-steering flash-rear
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
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-motor/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-steering-tof/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/firmware/rp2040-rear-tof/build
    COMMENT "Cleaning all build directories"
)