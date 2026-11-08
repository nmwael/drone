# STANAG S3 Simulator — ESP32-S3 POC

ESP32-S3 firmware that loops 5 JPEG frames, simulates GPS/IMU/Mag/Baro/AHT20 sensors, encodes to H.264, muxes into MPEG-TS with MISB ST 0601 KLV metadata, and streams via ESP-NOW/UDP for STANAG 4609 compliance testing.

## Features

- **5 JPEG frames** loaded from SPIFFS → HW JPEG decode → YUV420
- **H.264 hardware encoder** (High Profile L4.1, 1280×720 @ 30fps, 4 Mbps CBR)
- **Simulated sensors**: GPS (RTK), IMU (9-DoF), Magnetometer, Barometer, AHT20
- **MISB ST 0601 KLV** metadata per frame (UAS Local Set)
- **MPEG-TS muxer** (PID 0x100 video, PID 0x101 KLV)
- **ESP-NOW / UDP** streaming to ground station
- **UART telemetry** to ESP32-C3 (JSON format)

## Hardware

- ESP32-S3 DevKit (PSRAM required, 8MB+)
- USB cable (power + serial monitor)

## Quick Start

```bash
# Install ESP-IDF v5.2+
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
│   ├── jpeg_loader.c/.h       # SPIFFS JPEG load + HW decode
│   ├── sensor_sim.c/.h        # GPS/IMU/Mag/Baro/AHT20 simulators
│   ├── h264_encoder.c/.h      # HW H.264 encode
│   ├── klv_builder.c/.h       # MISB ST 0601 KLV
│   ├── ts_muxer.c/.h          # MPEG-TS muxer
│   ├── network_tx.c/.h        # ESP-NOW/UDP TX
│   ├── uart_telemetry.c/.h    # UART JSON to C3
│   └── CMakeLists.txt
├── data/
│   ├── frame_0.jpg .. frame_4.jpg
│   └── CMakeLists.txt         # SPIFFS image generation
├── components/
├── sdkconfig.defaults
├── partitions.csv
└── CMakeLists.txt
```

## Data Preparation

Place 10 JPEG frames (1280×720, ~85% quality, all day scenes) in `data/`:
- `frame_0.jpg` — Day scene 0 (straight road)
- `frame_1.jpg` — Day scene 1 (gentle left curve)
- `frame_2.jpg` — Day scene 2 (gentle right curve)
- `frame_3.jpg` — Day scene 3 (straight with distant trees)
- `frame_4.jpg` — Day scene 4 (intersection approach)
- `frame_5.jpg` — Day scene 5 (open road, horizon)
- `frame_6.jpg` — Day scene 6 (bridge approach)
- `frame_7.jpg` — Day scene 7 (urban street)
- `frame_8.jpg` — Day scene 8 (roundabout approach)
- `frame_9.jpg` — Day scene 9 (highway merge)

Run `idf.py build` — SPIFFS image auto-generated from `data/`.

## Simulated Sensors

| Sensor | Output | Update Rate |
|--------|--------|-------------|
| GPS (RTK) | Position, velocity, time, fix | 10 Hz |
| IMU | Accel, gyro, quaternion, Euler | 100 Hz |
| Mag | Heading, raw XYZ | 10 Hz |
| Baro | Pressure, altitude, temp | 10 Hz |
| AHT20 | Temp, humidity | 1 Hz |

All simulators produce realistic noise/drift patterns.

## KLV Metadata (MISB ST 0601)

Per-frame tags: 0x01-0x0F (UAS Local Set) + 0x4B-0x4E (Temp/RH/Pressure/Alt)

## Output Streams

1. **ESP-NOW / UDP** → MPEG-TS (Ground Station MediaMTX:8888)
2. **UART @ 115200** → JSON telemetry (to ESP32-C3)

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

## License

MIT