# SRT (Secure Reliable Transport) - Minimal ESP-IDF Implementation
# Implements a minimal SRT caller over lwIP UDP sockets

idf_component_register(
    SRCS "srt_stub.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_timer
        lwip
        log
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
)