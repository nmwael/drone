# ESP32-S3 Vision Firmware — STANAG 4609 + Thermal/IR + TFLite

**Status:** 🚧 **Planned / Scaffold** — Not yet implemented

Production vision pipeline for ESP32-S3 with OV5640 camera, hardware H.264 encoding, MISB ST 0601 KLV metadata, MPEG-TS streaming via ESP-NOW/UDP, and TensorFlow Lite Micro inference.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-S3 (Vision)                          │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │ OV5640 Day   │  │ Thermal/IR   │  │ Vision Inference     │  │
│  │ Camera (CSI) │  │ Camera       │  │ (TFLite Micro)       │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────┬───────────┘  │
│         │                 │                     │              │
│         ▼                 ▼                     ▼              │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Frame Processing Pipeline                    │  │
│  │  CSI → ISP → YUV420 → HW H.264 Enc → KLV → MPEG-TS       │  │
│  └──────────────────────────┬────────────────────────────────┘  │
│                             │                                   │
│                             ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Output Streams                               │  │
│  │  ESP-NOW/UDP → MediaMTX:8888 (STANAG 4609 TS + KLV)      │  │
│  │  UART @ 115200 → ESP32-C3 (Telemetry/Commands)           │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Planned Components (from CMakeLists.txt)

| Component | Source File | Purpose |
|-----------|-------------|---------|
| **Thermal Camera** | `main/thermal_camera.c` | Thermal imaging sensor interface |
| **IR Camera** | `main/ir_camera.c` | Infrared/night vision camera |
| **Vision Inference** | `main/vision_inference.c` | TFLite Micro object detection/tracking |
| **UART Comms** | `main/comms_uart.c` | Communication with ESP32-C3 hub |
| **H.264 Encoder** | `main/h264_encoder.c` | **Hardware** H.264 encoding (ESP32-S3 ENC) |
| **KLV Builder** | `main/klv_builder.c` | MISB ST 0601 metadata injection |
| **TS Muxer** | `main/ts_muxer.c` | MPEG-TS packetization (PAT/PMT/PES) |
| **WiFi Stream** | `main/wifi_stream.c` | ESP-NOW/UDP transmission to ground |

---

## Target Specifications

| Parameter | Target |
|-----------|--------|
| **MCU** | ESP32-S3 (PSRAM required, 8MB+) |
| **Framework** | ESP-IDF v5.5+ |
| **Day Camera** | OV5640 @ 1280×720 @ 30fps (CSI) |
| **Thermal/IR** | MLX90640 or OV5640 w/ IR filter removed |
| **Video Codec** | H.264 High Profile L4.1 (HW encoder) |
| **Container** | MPEG-TS (STANAG 4609 compliant) |
| **Metadata** | MISB ST 0601 KLV (UAS Local Set) |
| **Transport** | ESP-NOW (primary) + UDP (fallback) |
| **Inference** | TFLite Micro (ESP-DL) |
| **Telemetry** | UART @ 115200 to ESP32-C3 |

---

## Key Differences from STANAG S3 Simulator

| Feature | STANAG S3 Simulator | ESP32-S3 Vision (Planned) |
|---------|---------------------|---------------------------|
| **Video Source** | 5 JPEG frames (SPIFFS) | OV5640 CSI camera (live) |
| **H.264 Encoding** | Software fallback | **Hardware encoder** |
| **Thermal/IR** | ❌ Not implemented | ✅ Dual camera support |
| **AI Inference** | ❌ None | ✅ TFLite Micro (ESP-DL) |
| **KLV Tags** | 7 basic tags | Full MISB ST 0601 UAS Local Set |
| **Sensor Data** | Simulated | Real sensors via C3 hub |
| **Target Use** | Compliance testing | Production deployment |

---

## Build & Development

```bash
cd /workspaces/drone/firmware/esp32-s3-vision

# Set target (once)
idf.py set-target esp32s3

# Configure (menuconfig for camera, WiFi, etc.)
idf.py menuconfig

# Build
idf.py build

# Flash & monitor (when hardware ready)
idf.py -p /dev/ttyUSB0 flash monitor
```

### Required SDK Config (sdkconfig.defaults)
```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
# Camera config
CONFIG_CAMERA_ENABLE=y
CONFIG_OV5640_SUPPORT=y
# H.264 HW encoder
CONFIG_VIDEO_H264_ENC_HW=y
```

---

## Hardware Connections (Planned)

| Signal | ESP32-S3 GPIO | Notes |
|--------|---------------|-------|
| **CSI Camera** | CSI interface (dedicated) | OV5640 day camera |
| **Thermal/IR** | CSI or parallel | MLX90640 (I2C) or 2nd OV5640 |
| **UART to C3** | GPIO 17/18 (UART1) | 115200 baud, telemetry + commands |
| **ESP-NOW Antenna** | U.FL / PCB | 2.4 GHz for ESP-NOW |
| **Status LED** | GPIO 48 (RGB) | Built-in on DevKit |

---

## Development Status

| Component | Status | Notes |
|-----------|--------|-------|
| Project scaffold | ✅ CMakeLists.txt | Component list defined |
| Main entry | ✅ main.c | Task structure outlined |
| Thermal camera | 🚧 Placeholder | `thermal_camera.h` declared |
| IR camera | 🚧 Placeholder | `ir_camera.h` declared |
| Vision inference | 🚧 Placeholder | `vision_inference.h` declared |
| H.264 encoder | 🚧 Placeholder | `h264_encoder.h` declared |
| KLV builder | 🚧 Placeholder | `klv_builder.h` declared |
| TS muxer | 🚧 Placeholder | `ts_muxer.h` declared |
| WiFi stream | 🚧 Placeholder | `wifi_stream.h` declared |
| UART comms | 🚧 Placeholder | `comms_uart.h` declared |

---

## Next Implementation Steps

1. **Create component headers** (`.h` files) with API definitions
2. **Implement camera drivers** — OV5640 CSI + thermal sensor I2C
3. **Port H.264/KLV/TS from simulator** — Adapt for HW encoder
4. **Integrate ESP-DL + TFLite Micro** — Object detection model
5. **Implement ESP-NOW + UART comms** — C3 hub protocol
6. **Add unit tests** — Unity framework (like simulator)
7. **Create documentation** — API.md, BUILD.md, TESTING.md, etc.

---

## Related Documentation

| Document | Location |
|----------|----------|
| **AI Onboarding** | `AGENTS.md` (this directory) |
| **Quick Reference** | `QUICK-REFERENCE.md` (this directory) |
| **Root Project Docs** | `/workspaces/drone/AGENTS.md`, `QUICK-REFERENCE.md` |
| **STANAG S3 Simulator** | `../stanag-s3-simulator/` (reference implementation) |
| **ESP32-C3 Hub** | `../esp32-c3-hub/` (sensor fusion peer) |

---

## License

MIT — Same as parent project.