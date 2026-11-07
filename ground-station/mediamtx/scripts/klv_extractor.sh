#!/bin/bash
# KLV Extractor Hook for MediaMTX
# Extracts MISB ST 0601 KLV metadata from MPEG-TS stream
# Called on stream start with path as argument

set -e

STREAM_PATH="$1"
OUTPUT_DIR="./klv_output/${STREAM_PATH}"
mkdir -p "${OUTPUT_DIR}"

echo "[KLV Extractor] Starting for stream: ${STREAM_PATH}"
echo "[KLV Extractor] Output directory: ${OUTPUT_DIR}"

# Use ffprobe to extract KLV data packets
# PID 0x101 is typically used for KLV in STANAG streams
ffprobe -i "udp://0.0.0.0:8888" \
    -show_packets \
    -select_streams d:0 \
    -of json \
    -loglevel quiet \
    > "${OUTPUT_DIR}/klv_packets.json" &

FFPROBE_PID=$!

# Also save raw TS for post-processing
ffmpeg -i "udp://0.0.0.0:8888" \
    -c copy \
    -map 0:d \
    -f mpegts \
    "${OUTPUT_DIR}/klv_raw.ts" &

FFMPEG_PID=$!

echo "[KLV Extractor] Started ffprobe (PID: ${FFPROBE_PID}) and ffmpeg (PID: ${FFMPEG_PID})"

# Wait for signals to stop
trap "kill ${FFPROBE_PID} ${FFMPEG_PID} 2>/dev/null; exit 0" SIGTERM SIGINT

wait ${FFPROBE_PID} ${FFMPEG_PID}