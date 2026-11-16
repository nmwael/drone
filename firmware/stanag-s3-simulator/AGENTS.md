# AI Agent Instructions — STANAG S3 Simulator

## Quick Context

**This firmware is currently RUNNING on `/dev/ttyACM0`** — live STANAG 4609 telemetry output.

**Target:** ESP32-S3 with PSRAM (8MB+)  
**Framework:** ESP-IDF v5.5.6  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

---

## Architecture (30 fps Pipeline)

```
┌─────────────────────────────────────────────────────────────────────┐
│                         app_main() @ 30 fps                         │
├─────────────────────────────────────────────────────────────────────┤
│  1. sensor_sim_get_data()      → sensor_data_t (GPS/IMU/Mag/Baro)   │
│  2. jpeg_loader_get_next()     → jpeg_frame_t (SPIFFS frame_N.jpg)  │
│  3. jpeg_loader_decode()       → yuv420_frame_t (dummy YUV pattern) │
│  4. h264_encoder_encode()      → h264_nalu_t (SPS/PPS/IDR/AUD)     │
│  5. klv_builder_build()        → klv_packet_t (MISB ST 0601 KLV)    │
│  6. ts_muxer_mux()             → ts_packet_t (MPEG-TS 188B pkts)    │
│  7. network_tx_send()          → UDP/ESP-NOW → MediaMTX:8888        │
│  8. uart_telemetry_send()      → UART1 JSON @ 115200                │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Critical Files (Read These First)

| File | Purpose |
|------|---------|
| `main/main.c` | Entry point, 30fps loop, initialization sequence |
| `main/stanag_types.h` | **ALL data structures** — single source of truth |
| `main/sensor_sim.c` | Circular flight path simulation (Aarhus, 500m radius) |
| `main/klv_builder.c` | MISB ST 0601 KLV encoding (7 tags) |
| `main/ts_muxer.c` | MPEG-TS muxer (PAT/PMT/PES/Null) |
| `scripts/parse_ts_output.py` | KLV/TS parser for serial captures |

---

## Data Structures (from `stanag_types.h`)

```c
// Aggregated sensor readings per frame
typedef struct {
    gps_data_t   gps;   // lat/lon/alt/heading/vel/fix/sats
    imu_data_t   imu;   // accel/gyro/quat/euler (banked turn)
    mag_data_t   mag;   // heading + raw XYZ
    baro_data_t  baro;  // pressure/altitude/temp
    aht20_data_t aht20; // temp/humidity
} sensor_data_t;

// JPEG from SPIFFS (5 frames cycling)
typedef struct {
    uint8_t *data; size_t len; int width; int height; int index;
} jpeg_frame_t;

// YUV420 planar (1280x720) — dummy test pattern on S3 v5.5.6
typedef struct {
    uint8_t *y, *u, *v; int width, height, stride_y, stride_uv;
    uint64_t timestamp_us;
} yuv420_frame_t;

// H.264 NAL unit (with start code 0x00000001)
typedef struct {
    uint8_t *data; size_t len; int type; uint64_t timestamp_us; bool keyframe;
} h264_nalu_t;

// KLV packet (MISB ST 0601)
typedef struct {
    uint8_t *data; size_t len; uint64_t timestamp_us;
} klv_packet_t;

// MPEG-TS packets (concatenated 188-byte packets)
typedef struct {
    uint8_t *data; size_t count; size_t total_len; uint64_t timestamp_us;
} ts_packet_t;
```

---

## KLV Tags (MISB ST 0601 UAS Local Set)

| Key | Name | Format | Source |
|-----|------|--------|--------|
| 0x0001 | Position | deg7/deg7/mm (int32×3) | GPS |
| 0x0002 | Velocity NED | mm/s (int32×3) | GPS |
| 0x0003 | Platform Heading | deg5 (int32) | GPS/IMU |
| 0x0102 | Barometric Altitude | mm (int32) | Baro |
| 0x0103 | Temperature | int16 × 0.1°C | AHT20/BMP280 |
| 0x0104 | Magnetic Heading | deg5 (int32) | Mag |
| 0xFFFF | Timestamp | µs (uint32) | GPS TOW |

---

## MPEG-TS Structure

| PID | Stream | Content |
|-----|--------|---------|
| 0x0000 | PAT | Program Association Table (every frame) |
| 0x1000 | PMT | Program Map Table (every frame) |
| 0x0100 | Video | H.264 PES + PCR |
| 0x0101 | KLV | MISB ST 0601 (Private stream) |
| 0x1FFF | Null | Padding |

---

## Common AI Tasks

### 1. View Live Telemetry
```bash
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0
# Output: JSON per frame + TS_OUT hex dumps + ESP_LOG lines
```

### 2. Parse Captured Log
```bash
# Capture first
cat /dev/ttyACM0 > serial.log

# Then parse
python3 scripts/parse_ts_output.py serial.log
# Shows: TS packets, PIDs, KLV tags with parsed values
```

### 3. Build & Test
```bash
cd /workspaces/drone/firmware/stanag-s3-simulator

# Main app
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# Unit tests (64 tests on QEMU)
idf.py -DTEST_COMPONENT=1 build
timeout 60 idf.py -p /dev/null qemu monitor
```

### 4. Add a New KLV Tag
1. Add key to `KLV_KEYS` in `main/klv_builder.c`
2. Add encoding in `klv_builder_build()` (big-endian 2-byte key/len)
3. Update `parse_klv_value()` in `scripts/parse_ts_output.py`
4. Add test case in `test/klv_builder_test.c`
5. Update `API.md` KLV table and `DATA-FLOW.md` diagram

### 5. Modify Sensor Simulation
- Edit `main/sensor_sim.c` → `sensor_sim_get_data()`
- Circular path: 500m radius, Aarhus (56.1735°N, 10.2040°E), ~6.6s period
- Add fields to `sensor_data_t` in `stanag_types.h` if needed
- Update `sensor_sim_test.c` with new assertions

### 6. Change Frame Rate
- Modify `FRAME_RATE_HZ` in `main/main.c`
- Update `FRAME_INTERVAL_MS = 1000 / FRAME_RATE_HZ`
- Update timing budget in `DATA-FLOW.md` (Mermaid gantt chart)
- Adjust sensor update rates in `sensor_sim.c` if needed

---

## Build Configuration (Critical)

**`sdkconfig.defaults` must have:**
```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y   # Uses partitions.csv (has SPIFFS)
CONFIG_SPIRAM=y                    # PSRAM for YUV buffers
CONFIG_SPIRAM_MODE_OCT=y           # Octal PSRAM mode
CONFIG_SPIRAM_SPEED_40M=y          # 40 MHz (stable)
```

**`partitions.csv`:**
```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x100000,
spiffs,   data, spiffs,  0x110000, 0xEF000,  # ~960KB for 5 JPEGs
```

---

## Testing (64 Unity Tests)

| Module | Tests | File |
|--------|-------|------|
| KLV Builder | 8 | `test/klv_builder_test.c` |
| TS Muxer | 11 | `test/ts_muxer_test.c` |
| H.264 Encoder | 9 | `test/h264_encoder_test.c` |
| JPEG Loader | 10 | `test/jpeg_loader_test.c` |
| Sensor Sim | 14 | `test/sensor_sim_test.c` + `sensor_sim_extended_test.c` |
| Network TX | 5 | `test/network_tx_test.c` |
| UART Telemetry | 4 | `test/uart_telemetry_test.c` |

Run: `idf.py -DTEST_COMPONENT=1 build && timeout 60 idf.py -p /dev/null qemu monitor`

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| SPIFFS image fails | `CONFIG_PARTITION_TABLE_CUSTOM=n` | Set `=y` in `sdkconfig.defaults` |
| `esp_psram_*` undefined | `CONFIG_SPIRAM=n` | Set `=y` in `sdkconfig.defaults` |
| `idf.py` not found | ESP-IDF not sourced | `eim shell v5.5.6` |
| UDP errno 118 | No network (WSL/QEMU) | Expected; use Unix socket for local test |
| Frame overrun >33ms | Pipeline too slow | Profile; reduce frame rate or optimize |
| JPEG decode failed | No HW JPEG on S3 v5.5.6 | Expected; dummy YUV used |

---

## Key Conventions

- **Static buffers** for hot path (no malloc in 30fps loop)
- **PSRAM** for large allocations (`MALLOC_CAP_SPIRAM`)
- **ESP_LOGI(TAG, ...)** with module-specific TAG
- **Error handling:** `ESP_ERROR_CHECK()` for init, check returns in loop
- **Tests:** Unity, `setUp()`/`tearDown()`, `RUN_TEST()` in `test_runner.c`
- **Docs:** Update `API.md` when changing function signatures

---

## When in Doubt

1. **Read `main/stanag_types.h`** — all types in one place
2. **Check `test/test_runner.c`** — shows module init/usage patterns
3. **Run tests on QEMU** — validates without hardware
4. **Parse serial output** — `python3 scripts/parse_ts_output.py serial.log`
5. **Check `DATA-FLOW.md`** — Mermaid sequence diagram of pipeline