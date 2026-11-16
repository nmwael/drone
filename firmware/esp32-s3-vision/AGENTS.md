# AI Agent Instructions — ESP32-S3 Vision Firmware

## Quick Context

**Status:** 🚧 **Planned / Scaffold** — Not yet implemented  
**Target:** ESP32-S3 with PSRAM (8MB+)  
**Framework:** ESP-IDF v5.5+  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

---

## Planned Architecture

```
OV5640 (CSI) → ISP → YUV420 → HW H.264 Enc → NAL Units
                                                        ↓
Thermal/IR Sensor → Frame Buffer ─────────────────────►│
                                                        ▼
                                              KLV Builder → MISB ST 0601
                                                        ↓
                                              MPEG-TS Muxer → TS Packets
                                                        ↓
                                              ESP-NOW/UDP → MediaMTX:8888
                                                        ↓
                                              UART1 @ 115200 → ESP32-C3 Hub
```

### Parallel: Vision Inference (TFLite Micro)
```
Camera Frames → Preprocess → TFLite Micro (ESP-DL) → Detections
                                                            ↓
                                              Overlay on video / Send via UART
```

---

## Key Components (from CMakeLists.txt)

| Component | Header | Purpose |
|-----------|--------|---------|
| Thermal Camera | `thermal_camera.h` | Thermal imaging (MLX90640 or similar) |
| IR Camera | `ir_camera.h` | Night vision / IR camera |
| Vision Inference | `vision_inference.h` | TFLite Micro object detection |
| UART Comms | `comms_uart.h` | JSON/telemetry to ESP32-C3 |
| H.264 Encoder | `h264_encoder.h` | **Hardware** H.264 (ESP32-S3 ENC) |
| KLV Builder | `klv_builder.h` | MISB ST 0601 KLV metadata |
| TS Muxer | `ts_muxer.h` | MPEG-TS packetizer |
| WiFi Stream | `wifi_stream.h` | ESP-NOW/UDP to ground station |

---

## Critical Differences from STANAG S3 Simulator

| Aspect | Simulator | Vision (Planned) |
|--------|-----------|------------------|
| Video Source | 5 JPEG frames (SPIFFS) | **OV5640 CSI camera (live)** |
| H.264 Encoder | **Software fallback** | **Hardware encoder** (ENC peripheral) |
| Thermal/IR | ❌ None | ✅ Dual camera support |
| AI Inference | ❌ None | ✅ TFLite Micro (ESP-DL) |
| KLV Tags | 7 basic tags | Full MISB ST 0601 UAS Local Set |
| Sensor Data | Simulated locally | Real sensors via C3 hub (UART) |

---

## Planned Data Structures (to be defined in headers)

```c
// Camera frames
typedef struct {
    uint8_t *data;
    size_t len;
    int width, height;
    int format;        // YUV422, RGB565, etc.
    uint64_t timestamp_us;
} camera_frame_t;

// Vision inference results
typedef struct {
    int class_id;
    float confidence;
    float x, y, w, h;  // Normalized 0-1
} detection_t;

typedef struct {
    detection_t *detections;
    int count;
    uint64_t timestamp_us;
} inference_result_t;

// Telemetry to C3 hub
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

### 2. Adding Camera Support
- Implement `thermal_camera_init/start/stop` in `thermal_camera.c`
- Implement `ir_camera_init/start/stop` in `ir_camera.c`
- Use ESP-IDF `esp_camera` component for OV5640
- Use I2C for MLX90640 thermal sensor

### 3. Integrating TFLite Micro
- Add model (.tflite) to component resources
- Use ESP-DL APIs for quantization/acceleration
- Implement `vision_inference_process_frame()` in `vision_inference.c`

### 4. Hardware H.264 Encoder
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

# ESP-DL / TFLite Micro
CONFIG_ESP_DL_ENABLE=y
CONFIG_TFLITE_MICRO_ENABLE=y
```

### Partition Table (planned)
```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,  # 1.5MB for app + model
model,    data, fat,     0x190000, 0x80000,  # 512KB for TFLite model
```

---

## Testing Strategy (Planned)

| Test Type | Approach |
|-----------|----------|
| Unit tests | Unity framework (like simulator) |
| Camera integration | OV5640 + thermal on hardware |
| H.264 compliance | MediaMTX + ffprobe validation |
| KLV validation | `parse_ts_output.py` on serial capture |
| Inference accuracy | Test dataset with known objects |
| ESP-NOW latency | Measure glass-to-glass latency |

---

## Hardware Connections (Planned)

| Interface | GPIOs | Purpose |
|-----------|-------|---------|
| CSI Camera | Dedicated CSI pins | OV5640 day camera |
| Thermal I2C | GPIO 11/12 (I2C0) | MLX90640 |
| UART to C3 | GPIO 17/18 (UART1) | 115200 baud |
| ESP-NOW | WiFi radio (internal) | 2.4 GHz |
| Status LED | GPIO 48 (RGB) | Built-in DevKit LED |

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
2. **ESP-IDF docs** — Camera, Video Encoder, ESP-DL, ESP-NOW APIs
3. **MISB ST 0601** — Full tag definitions for KLV
4. **STANAG 4609** — MPEG-TS structure requirements