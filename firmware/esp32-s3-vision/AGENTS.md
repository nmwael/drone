# AI Agent Instructions — ESP32-S3 Vision Firmware

## Quick Context

**Status:** 🚧 **Planned / Scaffold** — Not yet implemented  
**Target:** ESP32-S3 with PSRAM (8MB+)  
**Framework:** ESP-IDF v5.5+  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

**Scope:** Day camera (OV5640) → HW H.264 → KLV → MPEG-TS → ESP-NOW/UDP. **No thermal, no night vision, no AI inference.**

---

## Planned Architecture

```
OV5640 (CSI) → ISP → YUV420 → HW H.264 Enc → NAL Units
                                                        ↓
                                              KLV Builder → MISB ST 0601
                                                        ↓
                                              MPEG-TS Muxer → TS Packets
                                                        ↓
                                              ESP-NOW/UDP → MediaMTX:8888
                                                        ↓
                                              UART1 @ 115200 → ESP32-C3 Hub
```

---

## Key Components (from CMakeLists.txt — Updated)

| Component | Header | Purpose |
|-----------|--------|---------|
| Day Camera | `day_camera.h` | OV5640 CSI camera driver |
| UART Comms | `comms_uart.h` | JSON/telemetry to ESP32-C3 |
| H.264 Encoder | `h264_encoder.h` | **Hardware** H.264 (ESP32-S3 ENC) |
| KLV Builder | `klv_builder.h` | MISB ST 0601 KLV metadata |
| TS Muxer | `ts_muxer.h` | MPEG-TS packetizer |
| WiFi Stream | `wifi_stream.h` | ESP-NOW/UDP to ground station |

**Removed from original plan:** Thermal camera, IR camera, Vision inference (TFLite)

---

## Critical Differences from STANAG S3 Simulator

| Aspect | Simulator | Vision (Planned) |
|--------|-----------|------------------|
| Video Source | 5 JPEG frames (SPIFFS) | **OV5640 CSI camera (live)** |
| H.264 Encoder | **Software fallback** | **Hardware encoder** (ENC peripheral) |
| KLV Tags | 7 basic tags | Full MISB ST 0601 UAS Local Set |
| Sensor Data | Simulated locally | Real sensors via C3 hub (UART) |

---

## Planned Data Structures (to be defined in headers)

```c
// Camera frame from OV5640
typedef struct {
    uint8_t *data;
    size_t len;
    int width, height;
    int format;        // YUV422 from CSI, converted to YUV420
    uint64_t timestamp_us;
} camera_frame_t;

// Telemetry to/from C3 hub
typedef struct {
    // Sensor data from C3 (received via UART)
    // Command/control to C3 (sent via UART)
} hub_telemetry_t;
```

---

## Common AI Tasks (When Implemented)

### 1. Porting from Simulator
```bash
# Reference implementations in ../stanag-s3-simulator/main/
# - klv_builder.c/h → Adapt for full MISB tag set
# - ts_muxer.c/h → Same MPEG-TS structure
# - h264_encoder.c/h → Replace SW fallback with HW ENC API
```

### 2. Adding Day Camera Support
- Implement `day_camera_init/start/stop/capture` in `day_camera.c`
- Use ESP-IDF `esp_camera` component for OV5640
- CSI → ISP → YUV422 → convert to YUV420 for encoder

### 3. Hardware H.264 Encoder
```c
// ESP-IDF v5.5+ HW encoder API
#include "esp_video.h"
#include "esp_codec.h"

// Configure: High Profile, Level 4.1, 1280x720, 30fps, CBR 4-6 Mbps
// GOP: 30 (IDR every 1s)
// Output: NAL units with start codes
```

---

## Build Configuration (Planned)

### Required `sdkconfig.defaults`
```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y

# Camera
CONFIG_CAMERA_ENABLE=y
CONFIG_OV5640_SUPPORT=y

# Video encoding
CONFIG_VIDEO_H264_ENC_HW=y
CONFIG_ESP_VIDEO_ENC_H264=y
```

### Partition Table (simplified — no model partition needed)
```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,  # 1.5MB for app
```

---

## Testing Strategy (Planned)

| Test Type | Approach |
|-----------|----------|
| Unit tests | Unity framework (like simulator) |
| Camera integration | OV5640 on hardware |
| H.264 compliance | MediaMTX + ffprobe validation |
| KLV validation | `parse_ts_output.py` on serial capture |
| ESP-NOW latency | Measure glass-to-glass latency |

---

## Hardware Connections (Planned)

| Interface | GPIOs | Purpose |
|-----------|-------|---------|
| CSI Camera | Dedicated CSI pins | OV5640 day camera |
| UART to C3 | GPIO 17/18 (UART1) | 115200 baud |
| ESP-NOW | WiFi radio (internal) | 2.4 GHz |
| Status LED | GPIO 48 (RGB) | Built-in DevKit LED |

**Removed:** Thermal I2C, second camera interface

---

## Reference Implementation

**Use `../stanag-s3-simulator/` as reference for:**
- KLV builder (MISB ST 0601 encoding)
- TS muxer (MPEG-TS PAT/PMT/PES)
- UART telemetry protocol
- Build system (CMake, partitions, sdkconfig)
- Testing (Unity + QEMU)
- Documentation structure

---

## When in Doubt

1. **Check simulator** — `../stanag-s3-simulator/main/` for working implementations
2. **ESP-IDF docs** — Camera, Video Encoder, ESP-NOW APIs
3. **MISB ST 0601** — Full tag definitions for KLV
4. **STANAG 4609** — MPEG-TS structure requirements