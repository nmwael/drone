# RTMP Diagnostic Plan for ESP32 Firmware

> **AI hint:** Historical diagnostic plan for the `errno 118 (ETIMEDOUT)` TCP connect failure caused by subnet routing, *not* RTMP protocol. The wire-format fixes are complete; this doc remains useful for network-reachability debugging. Related: `RTMP_E2E_TEST_PLAN.md`, `REMOTE_VALIDATION_PLAN.md`, `docs/rtmp_reference/RTMP_DEBUG_JOURNAL.md`.

## Problem
ESP32 fails to connect to MediaMTX at 172.160.27.93:1935 with `errno 118 (ETIMEDOUT)` on TCP `connect()`. 
- ESP32 on WiFi 192.168.1.x (byvej20)
- MediaMTX at 172.160.27.93:1935
- ffmpeg from devcontainer works → MediaMTX reachable from devcontainer
- **BUT**: ESP32 on 192.168.1.x cannot route to 172.160.27.x (different subnets, no routing)

## Root Cause Analysis
The `errno 118 (ETIMEDOUT)` on TCP `connect()` indicates the TCP SYN packet is not getting a SYN-ACK response. This is a **network routing/firewall issue**, not an RTMP protocol issue.

**Network topology:**
- ESP32: 192.168.1.73 (byvej20 WiFi) 
- MediaMTX: 172.160.27.93 (different subnet)
- No routing between 192.168.1.x and 172.160.27.x

## Diagnostic Plan

### Phase 1: Add RTMP Stage Diagnostics to Firmware
Add detailed logging at each RTMP stage in `network_tx_rtmp.c`:

1. **TCP Connect Stage** - Log before/after `connect()`, check `errno`
2. **RTMP Handshake** - Log C0/C1/C2 sent, S0/S1/S2 received
3. **Connect Command** - Log connect message sent, _result received
4. **CreateStream** - Log createStream sent, stream ID received
4. **Publish** - Log publish sent, onStatus received
5. **Video Send** - Log first video frame sent, check for ECONNRESET

### Phase 2: Reference Test in Devcontainer
- Test local MediaMTX (127.0.0.1:1935) - known working
- Test remote MediaMTX with ffmpeg - already works
- Compare packet captures

### Phase 3: Network Reachability Test
- Test if ESP32 can reach 172.160.27.93:1935 (TCP SYN/ACK)
- If not, run MediaMTX on ESP32's WiFi network (192.168.1.x)

## Implementation Steps

### 1. Add Diagnostic Logging to `network_tx_rtmp.c`
Add detailed ESP_LOGI at each stage with timestamps.

### 2. Create Reference Test
- Local MediaMTX at 127.0.0.1:1935 (working)
- Remote MediaMTX at 172.160.27.93:1935 (test with ffmpeg)

### 3. Network Fix
Run MediaMTX on ESP32's WiFi network (192.168.1.x) or set up routing.