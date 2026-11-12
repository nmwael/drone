# SPIFFS partition generation for JPEG frames - handled by project CMakeLists.txt
# This file just lists the images for reference

# List of JPEG files to embed in SPIFFS
set(SPIFFS_IMAGES
    frame_1.jpg
    frame_2.jpg
    frame_3.jpg
    frame_4.jpg
    frame_5.jpg
)

# Verify files exist
foreach(img ${SPIFFS_IMAGES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${img})
        message(FATAL_ERROR "Missing JPEG: ${img}")
    endif()
endforeach()