# Quick Reference — ESP32-S3 Vision Firmware

## ⚠️ Status: PLANNED / SCAFFOLD — Not Yet Implemented

---

## 🎯 Target Specs

| Parameter | Value |
|-----------|-------|
| MCU | ESP32-S3 (PSRAM 8MB+) |
| Framework | ESP-IDF v5.5+ |
| Day Camera | OV5640 @ 1280×720 @ 30fps (CSI) |
| Thermal/IR | MLX90640 (I2C) or 2nd OV5640 |
| Video Codec | H.264 High Profile L4.1 (**HW encoder**) |
| Container | MPEG-TS (STANAG 4609) |
| Metadata | MISB ST 0601 KLV (full UAS Local Set) |
| Transport | ESP-NOW (primary) + UDP → MediaMTX:8888 |
| Inference | TFLite Micro (ESP-DL) |
| Telemetry | UART1 @ 115200 → ESP32-C3 Hub |

---

## 📁 Project Structure

```
esp32-s3-vision/
├── CMakeLists.txt          # Component registration
├── sdkconfig.defaults      # Required Kconfig options
├── partitions.csv          # Partition table (app + model)
├── main/
│   ├── main.c              # Entry point, task setup
│   ├── thermal_camera.c/h  # Thermal imaging
│   ├── ir_camera.c/h       # IR/night vision
│   ├── vision_inference.c/h # TFLite Micro inference
│   ├── comms_uart.c/h      # UART to C3 hub
│   ├── h264_encoder.c/h    # HW H.264 encoder
│   ├── klv_builder.c/h     # MISB ST 0601 KLV
│   ├── ts_muxer.c/h        # MPEG-TS muxer
│   └── wifi_stream.c/h     # ESP-NOW/UDP TX
└── components/             # ESP-DL, TFLite Micro (managed)
```

---

## ⚡ Commands (When Ready)

```bash
cd /workspaces/drone/firmware/esp32-s3-vision

# Setup
idf.py set-target esp32s3
idf.py menuconfig

# Build
idf.py build

# Flash & Monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Size
idf.py size-components
```

---

## 🔑 Key SDK Config (sdkconfig.defaults)

```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_CAMERA_ENABLE=y
CONFIG_OV5640_SUPPORT=y
CONFIG_VIDEO_H264_ENC_HW=y
CONFIG_ESP_DL_ENABLE=y
CONFIG_TFLITE_MICRO_ENABLE=y
```

---

## 🔌 Hardware Connections (Planned)

| Interface | GPIO | Purpose |
|-----------|------|---------|
| CSI Camera | Dedicated | OV5640 day |
| I2C Thermal | 11/12 (I2C0) | MLX90640 |
| UART to C3 | 17/18 (UART1) | 115200 baud |
| ESP-NOW | WiFi Radio | 2.4 GHz |
| Status LED | 48 (RGB) | DevKit built-in |

---

## 🧩 Component APIs (To Be Defined)

| Component | Key Functions |
|-----------|---------------|
| `thermal_camera` | `init()`, `start()`, `capture()` |
| `ir_camera` | `init()`, `start()`, `capture()` |
| `vision_inference` | `init()`, `process_frame()`, `get_detections()` |
| `h264_encoder` | `init()`, `encode_frame()`, `get_nalu()` |
| `klv_builder` | `init()`, `build_klv(sensors, frame)` |
| `ts_muxer` | `init()`, `mux(nalu, klv)` |
| `wifi_stream` | `init()`, `send_ts()`, `set_dest()` |
| `comms_uart` | `init()`, `send_telemetry()`, `recv_commands()` |

---

## 📋 Implementation Checklist

- [ ] Create all `.h` headers with API definitions
- [ ] Implement OV5640 CSI driver (`thermal_camera`/`ir_camera`)
- [ ] Implement MLX90640 I2C driver (thermal)
- [ ] Port H.264 encoder from simulator → HW ENC API
- [ ] Port KLV builder from simulator → full MISB tag set
- [ ] Port TS muxer from simulator → same structure
- [ ] Implement ESP-NOW + UDP in `wifi_stream`
- [ ] Implement UART protocol to C3 hub in `comms_uart`
- [ ] Integrate ESP-DL + TFLite Micro in `vision_inference`
- [ ] Add model (.tflite) to partition/component
- [ ] Create unit tests (Unity, like simulator)
- [ ] Create documentation (API.md, BUILD.md, TESTING.md, etc.)

---

## 🔗 Reference: STANAG S3 Simulator

| Simulator File | Vision Adaptation |
|----------------|-------------------|
| `../stanag-s3-simulator/main/h264_encoder.c` | → HW encoder API (`esp_video_enc`) |
| `../stanag-s3-simulator/main/klv_builder.c` | → Full MISB ST 0601 tag set |
| `../stanag-s3-simulator/main/ts_muxer.c` | → Same MPEG-TS structure |
| `../stanag-s3-simulator/main/uart_telemetry.c` | → Bidirectional UART protocol |
| `../stanag-s3-simulator/main/network_tx.c` | → ESP-NOW + UDP |
| `../stanag-s3-simulator/test/` | → Unity test patterns |

---

## 📚 Key References

- **ESP-IDF Camera:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/camera.html
- **ESP-IDF Video Encoder:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/video_encoder.html
- **ESP-DL:** https://github.com/espressif/esp-dl
- **MISB ST 0601:** https://www.gwg.nga.mil/misb/docs/ST0601.pdf
- **STANAG 4609:** https://www.nato.int/stanag/