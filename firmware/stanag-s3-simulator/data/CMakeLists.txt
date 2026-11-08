# SPIFFS partition generation for JPEG frames

# List of JPEG files to embed in SPIFFS
set(SPIFFS_IMAGES
    frame_0.jpg
    frame_1.jpg
    frame_2.jpg
    frame_3.jpg
    frame_4.jpg
)

# Verify files exist
foreach(img ${SPIFFS_IMAGES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${img})
        message(FATAL_ERROR "Missing JPEG: ${img}")
    endif()
endforeach()

# Generate SPIFFS image using esp-idf's spiffsgen.py
# This runs at build time via idf_component_register
idf_component_register(
    SRCS
    INCLUDE_DIRS
    EMBED_FILES ${SPIFFS_IMAGES}
)