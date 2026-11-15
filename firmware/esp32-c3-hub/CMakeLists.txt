# ESP32-C3 Sensor Hub Firmware - CMakeLists.txt
# GPS + Mag + IMU + Baro + Temp/Humidity + Sensor Fusion EKF

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32-c3-hub)