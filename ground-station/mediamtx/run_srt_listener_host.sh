#!/bin/bash
# Run this on the HOST machine (not in devcontainer)
# Requires: ffmpeg with libsrt, or MediaMTX binary

set -e

echo "=== SRT Listener for ESP32 STANAG Stream ==="
echo "Host WiFi IP: $(ip route get 192.168.1.1 | awk '{print $7}')"
echo "Listening on: 0.0.0.0:8890 (SRT listener mode)"
echo ""

# Option 1: ffmpeg (if available with libsrt)
if command -v ffmpeg &> /dev/null; then
    echo "Starting ffmpeg SRT listener..."
    echo "Press Ctrl+C to stop"
    ffmpeg -i "srt://0.0.0.0:8890?mode=listener&latency=200" \
           -c copy \
           -f mpegts \
           "output_$(date +%Y%m%d_%H%M%S).ts"
else
    echo "ffmpeg not found. Trying MediaMTX..."
    if [ -f "./mediamtx" ]; then
        ./mediamtx mediamtx.yml
    else
        echo "Error: Neither ffmpeg nor MediaMTX found"
        echo "Install ffmpeg: sudo apt install ffmpeg"
        echo "Or download MediaMTX: wget https://github.com/bluenviron/mediamtx/releases/latest/download/mediamtx_v1.11.0_linux_amd64.tar.gz"
        exit 1
    fi
fi