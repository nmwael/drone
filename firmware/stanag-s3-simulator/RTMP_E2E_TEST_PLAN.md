# RTMP End-to-End Test Plan — ESP32 → MediaMTX (172.160.27.93:1935)

> **AI hint:** Comprehensive E2E test script: network reachability → build/flash → serial stage verification → MediaMTX/HLS checks → ECONNRESET debugging. Use as the runbook for any RTMP regression. Wire-format is considered correct; network is the remaining gate. Related: `REMOTE_VALIDATION_PLAN.md`, `RTMP_DIAGNOSTIC_PLAN.md`, `docs/rtmp_reference/RTMP_DEBUG_JOURNAL.md`.

**Status:** All wire-format fixes applied (per RTMP_DEBUG_JOURNAL.md). Network reachability is the only remaining blocker.

---

## 1. Network Connectivity Verification (Run FIRST)

### 1.1 From Host/DevContainer — Test MediaMTX Reachability
```bash
# Test TCP connectivity to MediaMTX RTMP port
nc -zv 172.160.27.93 1935

# If that fails, check if MediaMTX is actually listening on 0.0.0.0:1935
# From MediaMTX host:
ss -ltnp | grep :1935
# Should show: LISTEN 0 128 0.0.0.0:1935 0.0.0.0:* users:(("mediamtx",pid=...,fd=...))
```

**Expected:** `Connection to 172.160.27.93 1935 port [tcp/*] succeeded!`

### 1.2 From ESP32's WiFi Network (192.168.1.x) — Test Reachability
```bash
# From a device on the same WiFi as ESP32 (e.g., your laptop):
nc -zv 172.160.27.93 1935

# Trace route to identify where it breaks:
traceroute 172.160.27.93
```

### 1.3 If Unreachable — Fix Routing (Choose ONE)

**Option A: Run MediaMTX on ESP32's network (RECOMMENDED)**
```bash
# On a machine in 192.168.1.x (your laptop/desktop):
# 1. Copy mediamtx.yml to that machine
# 2. Run MediaMTX there:
./mediamtx mediamtx.yml

# 3. Update ESP32 config in main.c to point to that machine's 192.168.1.x IP:
network_tx_set_rtmp_config(
    "192.168.1.XXX", 1935,  /* Your laptop's IP on WiFi */
    "live", "sse_drone_f71919",
    "sse_drone_f71919", "5eefeb2389cf8b2ac83218ae926e7143"
);
```

**Option B: SSH Tunnel from DevContainer to MediaMTX Host**
```bash
# In devcontainer, forward local 1935 to remote MediaMTX:
ssh -L 1935:172.160.27.93:1935 user@mediamtx-host -N

# Then configure ESP32 to connect to devcontainer host IP (172.17.0.1 from container):
network_tx_set_rtmp_config("172.17.0.1", 1935, ...);
```

**Option C: Docker Network Bridge**
```bash
# Connect MediaMTX container to same Docker network as devcontainer
docker network connect bridge <mediamtx-container>
# Then MediaMTX reachable at container_name:1935
```

---

## 2. Build & Flash ESP32 (After Network Fixed)

```bash
cd /workspaces/drone/firmware/stanag-s3-simulator

# Clean build (ensures all fixes compiled)
idf.py fullclean
idf.py build

# Flash to hardware
idf.py -p /dev/ttyUSB0 flash monitor
# OR if using /dev/ttyACM0:
idf.py -p /dev/ttyACM0 flash monitor
```

---

## 3. Expected Serial Output — Stage by Stage

### Stage 1: Boot & WiFi (0-10s)
```
I (0) STANAG_POC: ========================================
I (0) STANAG_POC: STANAG S3 Simulator Starting
I (0) STANAG_POC: Free heap: 245672 bytes
I (0) STANAG_POC: Initializing SPIFFS...
I (0) STANAG_POC: SPIFFS initialized OK
...
I (0) STANAG_POC: WiFi started successfully
I (0) STANAG_POC: Connecting to WiFi...
I (0) STANAG_POC: Connected to AP
I (0) STANAG_POC: Got IP: 192.168.1.73
I (0) STANAG_POC: Netmask: 255.255.255.0
I (0) STANAG_POC: Gateway: 192.168.1.1
```

### Stage 2: RTMP Connection Attempt (every 2s until success)
```
I (10000) STANAG_POC: Retrying RTMP connection...
I (10000) NETWORK_TX_RTMP: RTMP: connecting to 172.160.27.93:1935 app=live stream=sse_drone_f71919 (attempt 1/5)
I (10000) NETWORK_TX_RTMP: RTMP handshake complete (S0+S1+S2 received)
I (10000) NETWORK_TX_RTMP: RTMP: stream ID from server: 1
I (10000) NETWORK_TX_RTMP: Using server-assigned stream_id=1
I (10000) NETWORK_TX_RTMP: RTMP: connected and publishing to live/sse_drone_f71919
I (10000) STANAG_POC: Frame 0 | GPS: 56.173500, 10.204000 | Alt: 100.0m | Hdg: 0.0° | Temp: 22.5°C
```

### Stage 3: First Video Frame (Critical - Was ECONNRESET)
```
I (10033) NETWORK_TX_RTMP: RTMP: First video frame sent successfully (timestamp=0 ms)
I (10066) NETWORK_TX_RTMP: RTMP: First video frame sent successfully (timestamp=33 ms)
I (10099) NETWORK_TX_RTMP: RTMP: First video frame sent successfully (timestamp=66 ms)
```

**If you see `RTMP: send failed, marking connection as dead` → ECONNRESET still happening (see Section 5)**

### Stage 4: Steady Streaming (30 fps)
```
I (11000) STANAG_POC: Frame 30 | GPS: 56.173500, 10.204000 | Alt: 100.0m | Hdg: 0.0° | Temp: 22.5°C
I (12000) STANAG_POC: Frame 60 | GPS: 56.173500, 10.204000 | Alt: 100.0m | Hdg: 0.0° | Temp: 22.5°C
...
```

---

## 4. Verify MediaMTX is Receiving the Stream

### 4.1 MediaMTX Logs (Run MediaMTX with `logLevel: debug` temporarily)
```bash
# In MediaMTX terminal, look for:
INFO  [RTMP] connection from 192.168.1.73:XXXXX
INFO  [RTMP] publishing stream "sse_drone_f71919"
INFO  [HLS] segment created for path "sse_drone_f71919"
```

### 4.2 MediaMTX Control API (if enabled: `api: yes` in mediamtx.yml)
```bash
# List active paths
curl http://172.160.27.93:9997/v3/paths/list

# Expected response:
# {
#   "items": [
#     {
#       "name": "sse_drone_f71919",
#       "ready": true,
#       "source": "publisher",
#       "sourceId": "...",
#       "tracks": [...]
#     }
#   ]
# }
```

### 4.3 HLS Playback Test (Port 8888)
```bash
# Check HLS manifest exists
curl -I http://172.160.27.93:8888/sse_drone_f71919/index.m3u8

# Should return 200 OK with m3u8 content
# Play with ffplay:
ffplay http://172.160.27.93:8888/sse_drone_f71919/index.m3u8
```

### 4.4 WebRTC Playback (Port 8889)
```bash
# Open in browser:
# http://172.160.27.93:8889/sse_drone_f71919
```

---

## 5. Debug Steps if ECONNRESET (errno 104) Still Occurs

### 5.1 Capture ESP32 Side PCAP
```bash
# On ESP32 host (Linux), capture ESP32 traffic:
sudo tcpdump -i any -w esp32_rtmp.pcap host 192.168.1.73 and port 1935

# Or if ESP32 on different interface, adjust -i
```

### 5.2 Capture MediaMTX Side PCAP
```bash
# On MediaMTX host:
sudo tcpdump -i any -w mediamtx_rtmp.pcap port 1935
```

### 5.3 Compare with FFmpeg Reference PCAP
```bash
# Generate reference stream from same H.264 source:
ffmpeg -re -f lavfi -i "testsrc=size=1280x720:rate=30" \
  -c:v libx264 -profile:v baseline -level 3.1 -g 30 \
  -f flv rtmp://172.160.27.93:1935/live/test_stream

# Capture during ffmpeg stream:
sudo tcpdump -i any -w ffmpeg_rtmp.pcap port 1935
```

### 5.4 Analyze PCAPs with Wireshark
```bash
# Key filters:
rtmp                    # All RTMP traffic
rtmp.type == 9          # Video messages
rtmp.type == 0x14       # Invoke commands
tcp.flags.reset == 1    # RST packets (ECONNRESET)
```

**Compare:**
- Chunk Stream IDs (CSID) for connect/createStream/publish/video
- Message Stream IDs in chunk headers
- NAL length prefix format (4-byte BE)
- onMetadata format (NOTIFY @setDataFrame)
- Server response sequence (WindowAck, SetPeerBW, StreamBegin, ChunkSize, _result)

### 5.5 Enable Verbose RTMP Logging on ESP32
Edit `network_tx_rtmp.c` temporarily:
```c
// Change log level for RTMP tag
#define TAG "NETWORK_TX_RTMP"
ESP_LOG_LEVEL_SET(TAG, ESP_LOG_DEBUG);  // Add in network_tx_rtmp_module_init()
```

Rebuild and watch for:
```
D (xxx) NETWORK_TX_RTMP: RTMP: write chunk: csid=5 type=9 len=XXXX ts=XXX stream_id=1
D (xxx) NETWORK_TX_RTMP: RTMP: send() returned XXX bytes
```

---

## 6. Success Criteria Checklist

| Stage | Verification | ✅ |
|-------|--------------|----|
| Network | `nc -zv 172.160.27.93 1935` succeeds from ESP32's network | ☐ |
| Handshake | Serial: `RTMP handshake complete (S0+S1+S2 received)` | ☐ |
| Connect | Serial: `RTMP: stream ID from server: X` | ☐ |
| CreateStream | Serial: `Using server-assigned stream_id=X` | ☐ |
| Publish | Serial: `RTMP: connected and publishing to live/sse_drone_f71919` | ☐ |
| onMetadata | MediaMTX logs show stream ready | ☐ |
| First Frame | Serial: `First video frame sent successfully` (NO ECONNRESET) | ☐ |
| Sustained | 30+ frames without reconnection | ☐ |
| HLS | `curl http://.../index.m3u8` returns segments | ☐ |
| Playback | ffplay/VLC plays stream smoothly | ☐ |

---

## 7. Quick Commands Reference

```bash
# Build & flash
cd /workspaces/drone/firmware/stanag-s3-simulator && idf.py build && idf.py -p /dev/ttyACM0 flash monitor

# Capture serial to file for parsing
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0 > serial.log

# Parse KLV/TS from serial log
python3 scripts/parse_ts_output.py serial.log

# MediaMTX API check
curl http://172.160.27.93:9997/v3/paths/list | jq .

# HLS manifest
curl http://172.160.27.93:8888/sse_drone_f71919/index.m3u8

# PCAP capture (run BEFORE starting ESP32)
sudo tcpdump -i any -w rtmp.pcap port 1935

# FFmpeg reference stream
ffmpeg -re -f lavfi -i "testsrc=size=1280x720:rate=30" -c:v libx264 -profile:v baseline -level 3.1 -g 30 -f flv rtmp://172.160.27.93:1935/live/test
```

---

## 8. If Network Still Unreachable — Fallback: Unix Socket Mode

For local testing without network routing:

```bash
# 1. MediaMTX config (add to mediamtx.yml paths):
#   stanag:
#     source: unix+mpegts:///tmp/stanag.sock

# 2. In main.c, enable Unix socket:
network_tx_set_unix_socket("/tmp/stanag.sock");
network_tx_set_mode(NETWORK_TX_MODE_UDP);  // Uses Unix socket when set

# 3. Run MediaMTX, then ESP32 - no network needed
```

---

**Next Action for Main Agent:**
1. **First:** Fix network reachability (Section 1.3 - pick Option A/B/C)
2. **Then:** Build, flash, and monitor serial output (Section 2-3)
3. **Verify:** MediaMTX receives stream (Section 4)
4. **If ECONNRESET:** Capture PCAPs and compare with ffmpeg (Section 5)

The wire-format is correct. Network is the only gate.