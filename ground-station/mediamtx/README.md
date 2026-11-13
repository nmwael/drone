# MediaMTX Ground Station — STANAG 4609 Testing

MediaMTX is a lightweight media server that ingests the STANAG 4609 MPEG-TS
stream from the ESP32-S3 simulator and serves it via WebRTC for browser/Oculus
viewing.

## Quick Start

```bash
cd /workspace/ground-station/mediamtx

# Start MediaMTX in the background
./start.sh background

# Check status
./start.sh status

# Stop
./start.sh stop
```

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 8888 | UDP | STANAG MPEG-TS ingest from ESP32-S3 |
| 8889 | TCP/UDP | WebRTC signaling + media (Oculus Quest browser) |
| 8554 | TCP | RTSP (control uplink) |
| 9997 | HTTP | MediaMTX REST API |
| 9998 | HTTP | Prometheus metrics |

## Testing with the ESP32-S3 Simulator

The simulator runs on QEMU or hardware and outputs TS packets. Since QEMU
doesn't have WiFi, the simulator's `network_tx.c` skips UDP/ESP-NOW sending.
To test end-to-end in QEMU, use the test sender which reads TS hex dumps
from the simulator's serial output and forwards them via UDP to MediaMTX.

### Option 1: Test from a captured serial log

```bash
# Capture simulator output to a log file
idf.py -p /dev/null qemu monitor 2>&1 | tee serial.log

# Run the test orchestrator with the log file
cd /workspace/ground-station/mediamtx
./test-orchestrator.sh --file serial.log
```

### Option 2: Real-time pipe from QEMU

```bash
# Terminal 1: Start MediaMTX
cd /workspace/ground-station/mediamtx
./start.sh background

# Terminal 2: Run simulator and pipe to test sender
cd /workspace/firmware/stanag-s3-simulator
idf.py -p /dev/null qemu monitor 2>&1 | \
  python3 ../ground-station/mediamtx/test-sender.py \
    --udp 127.0.0.1:8888 --delay 33
```

### Option 3: Manual testing

```bash
# Start MediaMTX
./start.sh background

# Verify it's running
curl http://localhost:9997/v3/paths/list

# Open WebRTC in browser
# http://localhost:8889 -> click "stanag" -> click Play
```

## Verifying the Stream

```bash
# List active paths/streams
curl http://localhost:9997/v3/paths/list

# Check if stanag path has a source connected
curl -s http://localhost:9997/v3/paths/list | python3 -c "
import json, sys
data = json.load(sys.stdin)
for item in data.get('items', []):
    print(f\"Path: {item['name']}, Source: {item.get('source', 'N/A')}, Ready: {item.get('ready', False)}\")
"

# View recordings (if enabled)
ls -la recordings/stanag/
```

## WebRTC Viewing (Oculus Quest / Browser)

1. Start MediaMTX: `./start.sh background`
2. Open browser: `http://localhost:8889`
3. Click on the `stanag` path
4. Click "Play"
5. For Oculus Quest: Use Quest Browser, enable WebRTC flags if needed
   - `chrome://flags/#unsafely-treat-insecure-origin-as-secure` → add `http://<IP>:8889`

## KLV Extraction

MediaMTX can run the KLV extractor hook on stream start:

```yaml
# mediamtx.yml
runOnInit: ./scripts/klv_extractor.sh %path
runOnInitRestart: true
```

The extractor saves raw TS and KLV packets to `klv_output/stanag/`.

## Files

| File | Purpose |
|------|---------|
| `mediamtx` | MediaMTX binary (v1.21.3) |
| `mediamtx.yml` | Project configuration (STANAG 4609) |
| `mediamtx.yml.sample` | Original MediaMTX sample config |
| `start.sh` | Start/stop/status wrapper |
| `test-sender.py` | Forward TS hex dumps from QEMU to MediaMTX via UDP |
| `test-orchestrator.sh` | Combined start MediaMTX + test sender |
| `scripts/klv_extractor.sh` | KLV extraction hook for MediaMTX |
| `mediamtx-test.log` | Log file (created on start) |
| `mediamtx.pid` | PID file (created on background start) |

## Stopping and Cleanup

```bash
./start.sh stop
rm -f mediamtx.pid mediamtx-test.log
rm -rf recordings/ klv_output/
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| MediaMTX won't start | Check `mediamtx-test.log` for errors |
| No stream received | Verify simulator is sending to `127.0.0.1:8888` |
| WebRTC won't connect | Use HTTPS or Chrome insecure origin flag |
| Path not found | Ensure `sourceOnDemand: false` in config |
