#!/bin/bash
# SRT Traffic Capture Script - Run on host machine
# Captures ESP32 <-> 172.160.27.93:8890 SRT handshake traffic

set -e

INTERFACE="${1:-wlan0}"
TARGET_IP="172.160.27.93"
TARGET_PORT="8890"
OUTPUT_FILE="srt_capture_$(date +%Y%m%d_%H%M%S).pcap"

echo "=== SRT Traffic Capture ==="
echo "Interface: $INTERFACE"
echo "Target: $TARGET_IP:$TARGET_PORT"
echo "Output: $OUTPUT_FILE"
echo ""

# Check if running as root (needed for tcpdump)
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: tcpdump requires root privileges"
    echo "Run with: sudo $0 [interface]"
    exit 1
fi

# Check interface exists
if ! ip link show "$INTERFACE" >/dev/null 2>&1; then
    echo "ERROR: Interface $INTERFACE not found"
    echo "Available interfaces:"
    ip link show | grep -E '^[0-9]+:' | awk '{print "  " $2}' | sed 's/:$//'
    exit 1
fi

echo "Starting capture... Press Ctrl+C to stop"
echo ""

# Capture with tcpdump
# -i: interface
# -n: don't resolve hostnames
# -s 0: capture full packets
# -w: write to pcap file
# Filter: UDP port 8890 to/from target IP
tcpdump -i "$INTERFACE" -n -s 0 -w "$OUTPUT_FILE" \
    "udp port $TARGET_PORT and host $TARGET_IP" 2>&1 | \
    while IFS= read -r line; do
        echo "$line"
    done

echo ""
echo "Capture saved to: $OUTPUT_FILE"
echo ""
echo "To analyze with Wireshark:"
echo "  wireshark $OUTPUT_FILE"
echo ""
echo "To analyze with tshark (CLI):"
echo "  tshark -r $OUTPUT_FILE -Y 'srt' -V"
echo ""
echo "To extract SRT stream IDs:"
echo "  tshark -r $OUTPUT_FILE -Y 'srt.streamid' -T fields -e srt.streamid"