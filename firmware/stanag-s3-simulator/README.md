# STANAG S3 Simulator — ESP32-S3 POC

ESP32-S3 firmware that loops 5 JPEG frames, simulates GPS/IMU/Mag/Baro/AHT20 sensors, encodes to H.264, muxes into MPEG-TS with MISB ST 0601 KLV metadata, and streams via ESP-NOW/UDP for STANAG 4609 compliance testing.

## Features

- **5 JPEG frames** loaded from SPIFFS → SW JPEG decode (stub) → YUV420
- **H.264 encoder** (High Profile L4.1, 1280×720 @ 30fps) — SW fallback with valid SPS/PPS/IDR NAL units
- **Simulated sensors**: GPS (RTK), IMU (9-DoF), Magnetometer, Barometer, AHT20
- **MISB ST 0601 KLV** metadata per frame (UAS Local Set + custom tags)
- **MPEG-TS muxer** with PAT/PMT/PES (PID 0x0000 PAT, 0x1000 PMT, 0x0100 video, 0x0101 KLV)
- **ESP-NOW / UDP** streaming to ground station
- **UART telemetry** to ESP32-C3 (JSON format, 115200 baud)
- **PSRAM support** enabled for large frame buffers
- **Unit tests** for all major modules (Unity framework)

## Hardware

- ESP32-S3 DevKit (PSRAM required, 8MB+)
- USB cable (power + serial monitor)
- Optional: ESP32-C3 for UART telemetry reception

## Quick Start

```bash
# Install ESP-IDF v5.5+
. $HOME/esp/esp-idf/export.sh

# Build
idf.py set-target esp32s3
idf.py build

# Flash & monitor
idf.py flash monitor
```

## Project Structure

```
stanag-s3-simulator/
├── main/
│   ├── main.c                 # Entry point, main loop
│   ├── jpeg_loader.c/.h       # SPIFFS JPEG load + SW decode (stub)
│   ├── sensor_sim.c/.h        # GPS/IMU/Mag/Baro/AHT20 simulators
│   ├── h264_encoder.c/.h      # H.264 encode (SW fallback with valid NAL)
│   ├── klv_builder.c/.h       # MISB ST 0601 KLV
│   ├── ts_muxer.c/.h          # MPEG-TS muxer (PAT/PMT/PES)
│   ├── network_tx.c/.h        # ESP-NOW/UDP TX
│   ├── uart_telemetry.c/.h    # UART JSON to C3
│   └── CMakeLists.txt
├── test/
│   ├── test_runner.c          # Unity test runner
│   ├── *_test.c               # Unit tests for each module
│   └── CMakeLists.txt
├── data/
│   ├── frame_1.jpg .. frame_5.jpg
│   └── CMakeLists.txt         # SPIFFS image generation
├── scripts/
│   ├── klv_extractor.sh       # KLV extraction hook for MediaMTX
│   └── parse_ts_output.py     # TS/KLV parser for serial logs
├── .github/workflows/
│   └── ci.yml                 # GitHub Actions CI/CD
├── sdkconfig                  # ESP-IDF config (PSRAM enabled)
├── partitions.csv             # Partition table (SPIFFS ~960KB)
├── CMakeLists.txt
├── MEDIAMTX.md                # Ground station documentation
└── README.md
```

## Data Preparation

Place 5 JPEG frames (1280×720, ~85% quality, all day scenes) in `data/`:
- `frame_1.jpg` — Day scene 0 (straight road)
- `frame_2.jpg` — Day scene 1 (gentle left curve)
- `frame_3.jpg` — Day scene 2 (gentle right curve)
- `frame_4.jpg` — Day scene 3 (straight with distant trees)
- `frame_5.jpg` — Day scene 4 (intersection approach)

Run `idf.py build` — SPIFFS image auto-generated from `data/`.

## Simulated Sensors

| Sensor | Output | Update Rate |
|--------|--------|-------------|
| GPS (RTK) | Position, velocity, time, fix | 10 Hz |
| IMU | Accel, gyro, quaternion, Euler | 100 Hz |
| Mag | Heading, raw XYZ | 10 Hz |
| Baro | Pressure, altitude, temp | 10 Hz |
| AHT20 | Temp, humidity | 1 Hz |

All simulators produce realistic noise/drift patterns (circular flight path around Systematic Aarhus, 500m radius).

## KLV Metadata (MISB ST 0601)

Per-frame tags:
- `0x0001` — Position (Lat/Lon/Height, deg7/mm)
- `0x0002` — Velocity NED (mm/s)
- `0x0003` — Platform Heading (deg5)
- `0x0102` — Barometric Altitude (mm)
- `0x0103` — Platform Temperature (int16 × 0.1°C)
- `0x0104` — Magnetic Heading (deg5)
- `0xFFFF` — Timestamp (μs)

## MPEG-TS Structure

| PID | Stream Type | Description |
|-----|-------------|-------------|
| 0x0000 | PAT | Program Association Table |
| 0x1000 | PMT | Program Map Table |
| 0x0100 | 0x1B (H.264) | Video PES with PCR |
| 0x0101 | 0x06 (Private) | KLV PES |
| 0x1FFF | — | Null packets (padding) |

## Output Streams

1. **ESP-NOW / UDP** → MPEG-TS (Ground Station MediaMTX:8888)
2. **UART @ 115200** → JSON telemetry (to ESP32-C3, pins TX=17, RX=18)

## Ground Station

See **[MediaMTX Ground Station Documentation](MEDIAMTX.md)** for complete setup including:
- MediaMTX installation (binary/Docker)
- Complete `mediamtx.yml` configuration for STANAG 4609
- WebRTC setup for Meta Quest 2/3
- KLV extraction hook
- Port reference, testing, troubleshooting

### Quick Config

```bash
# MediaMTX config (see mediamtx.yml for full config)
paths:
  stanag:
    source: udp://0.0.0.0:8888
    webrtc: true
```

Open `http://<ground-ip>:8889` in browser or Meta Quest 2 browser.

### Files
- `mediamtx.yml` — Ready-to-use MediaMTX config
- `scripts/klv_extractor.sh` — KLV extraction hook
- `MEDIAMTX.md` — Full documentation

## Testing

### Unit Tests (Unity)

```bash
# Build test app
idf.py -DTEST_COMPONENT=1 build

# Run on QEMU
idf.py -p /dev/null qemu monitor
```

Test coverage:
- KLV Builder (encoding, keys, values)
- TS Muxer (PAT/PMT/PES, PIDs, continuity counters)
- H.264 Encoder (SPS/PPS/IDR, start codes, timestamps)
- JPEG Loader (SPIFFS, frame cycling, YUV layout)
- Sensor Simulator (circular path, sensor ranges, correlations)
- Network TX (UDP/ESP-NOW config, send)
- UART Telemetry (JSON format, init/deinit)

### TS/KLV Analysis

```bash
# Capture serial output to file
idf.py monitor > serial.log

# Parse TS packets and KLV
python3 scripts/parse_ts_output.py serial.log
```

## CI/CD

Woodpecker CI (`.woodpecker.yml`) for Codeberg includes:
- Build verification for ESP32-S3
- QEMU emulation testing
- Unit test execution (Unity framework)
- Static analysis (cppcheck)
- MPEG-TS structure validation
- Binary size reporting

Woodpecker is the CI system used by Codeberg (Drone CI compatible syntax).

## Configuration

Key `sdkconfig` settings:
- `CONFIG_SPIRAM=y` — PSRAM enabled (required for YUV buffers)
- `CONFIG_SPIRAM_MODE_OCT=y` — Octal PSRAM mode
- `CONFIG_SPIRAM_SPEED_80M=y` — 80 MHz PSRAM
- `CONFIG_ESP32S3_SPIRAM_SUPPORT=y` — ESP32-S3 PSRAM support
- Partition table: Custom (`partitions.csv`) with ~960KB SPIFFS

## Hardware Testing Notes

For hardware testing (not QEMU):

1. Uncomment WiFi/ESP-NOW initialization in `main.c`
2. Call `network_tx_espnow_init()` after WiFi starts
3. Configure UDP destination: `network_tx_set_udp_dest("192.168.4.2", 8888)`
4. Configure ESP-NOW peer MAC: `network_tx_set_espnow_peer(mac_addr)`
5. Connect UART1 (GPIO17/18) to ESP32-C3 for telemetry

## License

MIT# CI trigger
# Workflow trigger Thu Nov 12 09:14:48 PM CET 2026
# CI trigger Thu Nov 12 09:28:05 PM CET 2026
# CI trigger Thu Nov 12 09:34:43 PM CET 2026
# CI trigger Thu Nov 12 09:48:23 PM CET 2026
# CI trigger Fri Nov 13 06:38:59 AM CET 2026
# CI trigger Fri Nov 13 06:46:43 AM CET 2026
# CI trigger Fri Nov 13 06:55:24 AM CET 2026
# CI trigger Fri Nov 13 07:04:26 AM CET 2026
# CI trigger Fri Nov 13 07:10:30 AM CET 2026
# CI trigger Fri Nov 13 07:17:10 AM CET 2026
