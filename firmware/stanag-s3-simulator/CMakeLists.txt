cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(stanag-s3-simulator)

# Generate SPIFFS image from data/ JPEG frames using ESP-IDF's built-in function
# This creates the SPIFFS image and flashes it to the 'spiffs' partition
spiffs_create_partition_image(spiffs ${CMAKE_SOURCE_DIR}/data FLASH_IN_PROJECT DEPENDS data/frame_1.jpg data/frame_2.jpg data/frame_3.jpg data/frame_4.jpg data/frame_5.jpg)