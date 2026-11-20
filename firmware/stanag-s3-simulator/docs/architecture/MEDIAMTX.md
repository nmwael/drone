# MediaMTX Ground Station Configuration

MediaMTX is a lightweight, zero-dependency media server that ingests the STANAG 4609 MPEG-TS stream from the ESP32-S3 and serves it via WebRTC to browsers (including Meta Quest 2/3).

## Quick Start

### Install (Linux/macOS/Windows)
```bash
# Download latest release
wget https://github.com/bluenviron/mediamtx/releases/latest/download/mediamtx_v1.11.0_linux_amd64.tar.gz
tar -xzf mediamtx_v1.11.0_linux_amd64.tar.gz
./mediamtx mediamtx.yml
```

Or with Docker:
```bash
docker run --rm -it \
  -p 8888:8888/udp \
  -p 8889:8889 \
  -p 8554:8554 \
  -v $(pwd)/mediamtx.yml:/mediamtx.yml \
  bluenviron/mediamtx:latest
```

### Run with Config
```bash
./mediamtx mediamtx.yml
```

## Configuration (`mediamtx.yml`)

```yaml
# MediaMTX configuration for STANAG 4609 stream from ESP32-S3

# Global settings
logLevel: info
logFormat: text
logFile: ""
api: true
apiAddress: :9997

# Paths
paths:
  # Main STANAG stream from ESP32-S3 via UDP
  stanag:
    # Ingest from ESP32-S3 (ESP-NOW bridge or direct WiFi)
    source: udp://0.0.0.0:8888
    sourceFormat: mpegts
    sourceOnDemand: false
    sourceRedirect: false

    # WebRTC output for Meta Quest 2/3 browser
    webrtc: true
    webrtcTransport: udp
    webrtcICEServers:
      - urls: stun:stun.l.google.com:19302
      - urls: stun:stun1.l.google.com:19302
      - urls: stun:stun2.l.google.com:19302
      # Add TURN server for NAT traversal if needed:
      # - urls: turn:your-turn-server.com:3478
      #   username: user
      #   credential: pass

    # HLS output (optional, for debugging)
    hls: false

    # Recording STANAG TS + KLV
    record: true
    recordPath: ./recordings/%path/%Y-%m-%d_%H-%M-%S.ts
    recordFormat: mpegts
    recordDeleteAfter: 24h

    # KLV extraction hook
    runOnInit: ./scripts/klv_extractor.sh %path
    runOnInitRestart: true

  # Telemetry downlink (MAVLink/JSON over UDP)
  telemetry:
    source: udp://0.0.0.0:8889
    sourceFormat: raw
    sourceOnDemand: false

  # Control uplink (WebSocket from Quest)
  control:
    source: rtsp://0.0.0.0:8554/control
    sourceOnDemand: true

# Authentication (disable for local dev)
auth:
  methods: [none]

# RTSP server (for control uplink)
rtsp:
  enable: true
  port: 8554

# WebRTC settings
webrtc:
  enable: true
  port: 8889
  iceHostNAT1To1IPs: ""
  iceHostNAT1To1Ports: ""
  allowOrigin: "*"

# HLS settings
hls:
  enable: false

# SRT settings (backup for long range)
srt:
  enable: false

# Metrics (Prometheus)
metrics:
  enable: true
  address: :9998
```

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 8888 | UDP | STANAG MPEG-TS input from ESP32-S3 |
| 8889 | TCP/UDP | WebRTC signaling + media |
| 8554 | TCP | RTSP (control uplink) |
| 9997 | HTTP | API / metrics |
| 9998 | HTTP | Prometheus metrics |

## Meta Quest 2/3 Setup

### Browser Access
1. Connect Quest to same WiFi as MediaMTX
2. Open **Meta Quest Browser** (or Firefox Reality)
3. Navigate to: `http://<MEDIAMTX_IP>:8889`
4. Click "Play" on the `stanag` stream
5. Click VR headset icon → "Enter VR"

### WebRTC Requirements
- **HTTPS required** for WebRTC in production
- For local testing, use `chrome://flags/#unsafely-treat-insecure-origin-as-secure` → add `http://<MEDIAMTX_IP>:8889`
- Or generate self-signed cert and use MediaMTX `https` config

## KLV Extraction Script

Create `scripts/klv_extractor.sh`:

```bash
#!/bin/bash
# KLV Extractor Hook for MediaMTX
# Extracts MISB ST 0601 KLV metadata from MPEG-TS stream

set -e

STREAM_PATH="$1"
OUTPUT_DIR="./klv_output/${STREAM_PATH}"
mkdir -p "${OUTPUT_DIR}"

echo "[KLV Extractor] Starting for stream: ${STREAM_PATH}"
echo "[KLV Extractor] Output directory: ${OUTPUT_DIR}"

# Extract KLV data packets (PID 0x101) using ffprobe
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
```

Make executable:
```bash
chmod +x scripts/klv_extractor.sh
```

## Testing

### 1. Verify MediaMTX is Running
```bash
# Check API
curl http://localhost:9997/v3/paths/list

# Expected response:
# {"items":[{"name":"stanag","confName":"stanag","source":"udp://0.0.0.0:8888",...}]}
```

### 2. Test with ESP32-S3 Simulator
```bash
# On ESP32-S3 (or simulator)
# Should stream to udp://<MEDIAMTX_IP>:8888
# Check MediaMTX logs:
# "path 'stanag' has been created"
# "source 'udp' connected"
```

### 3. Verify WebRTC in Browser
Open `http://<MEDIAMTX_IP>:8889` → Click `stanag` → Click Play → Video should appear

### 4. Verify KLV Extraction
```bash
ls -la klv_output/stanag/
# Should contain: klv_packets.json, klv_raw.ts
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No video in browser | Check UDP 8888 firewall; verify ESP32 sending to correct IP |
| WebRTC connection fails | Use HTTPS or Chrome insecure origin flag; check STUN servers |
| High latency | Reduce GOP size on ESP32-S3; use UDP transport; local network only |
| KLV not extracted | Verify PID 0x101 in TS; check ffprobe/ffmpeg in PATH |
| Quest can't connect | Same WiFi; Quest browser supports WebRTC; try Firefox Reality |

## Production Checklist

- [ ] Generate SSL certs for HTTPS WebRTC
- [ ] Configure TURN server for NAT traversal
- [ ] Set up systemd service for auto-start
- [ ] Configure log rotation
- [ ] Set up Prometheus + Grafana for metrics
- [ ] Configure recording retention policy
- [ ] Test with actual ESP32-S3 hardware
- [ ] Validate STANAG 4609 compliance (ffprobe TS structure)

## Resources

- **MediaMTX Docs:** https://github.com/bluenviron/mediamtx
- **MISB ST 0601:** https://www.gwg.nga.mil/misb/docs/ST0601.pdf
- **STANAG 4609:** https://www.nato.int/stanag/
- **WebRTC on Quest:** https://developer.oculus.com/documentation/webxr/webxr-browser/