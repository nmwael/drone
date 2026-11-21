# AI Agent Instructions — STANAG S3 Simulator

## Quick Context

**This firmware is currently RUNNING on `/dev/ttyACM0`** — live STANAG 4609 telemetry output.

**Target:** ESP32-S3 with PSRAM (8MB+)  
**Framework:** ESP-IDF v5.5.6  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

**Transport Status:**
- **RTMP** (port 1935): 🚧 **BASELINE** — Connects and publishes, but video DATA packets rejected by MediaMTX (errno 104). Primary focus for getting streaming working.
- **SRT** (port 8890): ❌ **INCOMPLETE** — Handshake completes but DATA packets rejected by MediaMTX. Deprioritized until RTMP baseline works.

---

## AI Agent Workflow

### Main Agent Orchestrates

**The primary AI agent (you're talking to me) orchestrates all work:**

1. **Direct execution** - I write code, edit files, run builds, flash firmware, analyze logs
2. **Spawn subagents** for specialized tasks:
   - **architect** - complex planning, root cause analysis, task decomposition
   - **coder** - large implementation tasks, new features
   - **explorer** - codebase searches, pattern finding
   - **researcher** - deep investigation, documentation review
   - **reviewer** - code reviews, security audits

3. **Subagents report back** with findings/plans, main agent executes

### Example: RTMP Fix (This Session)

| Step | Agent | Action |
|------|-------|--------|
| 1 | Main | Read debug journal, identified ECONNRESET issue |
| 2 | Architect | Analyzed ffmpeg reference, found 3 wire-format bugs |
| 3 | Main | Implemented all 3 fixes in `network_tx_rtmp.c` |
| 4 | Main | Updated `RTMP_DEBUG_JOURNAL.md` with root cause |
| 5 | Main | Built (`idf.py build`), flashed (`idf.py flash`), tested |

**Build system:** `idf.py` (ESP-IDF/CMake/Ninja) - invoked directly by main agent via shell.

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
│  7. network_tx_send()          → RTMP → MediaMTX:1935                 │
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
5. Update `docs/reference/API.md` KLV table and `docs/architecture/DATA-FLOW.md` diagram

### 5. Modify Sensor Simulation
- Edit `main/sensor_sim.c` → `sensor_sim_get_data()`
- Circular path: 500m radius, Aarhus (56.1735°N, 10.2040°E), ~6.6s period
- Add fields to `sensor_data_t` in `stanag_types.h` if needed
- Update `sensor_sim_test.c` with new assertions

### 6. Change Frame Rate
- Modify `FRAME_RATE_HZ` in `main/main.c`
- Update `FRAME_INTERVAL_MS = 1000 / FRAME_RATE_HZ`
- Update timing budget in `docs/architecture/DATA-FLOW.md` (Mermaid gantt chart)
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
| RTMP send() errno 104 | MediaMTX rejects video data | Check ffmpeg RTMP reference in docs/rtmp_reference/ffmpeg_rtmp_proto.c |
| RTMP connect() errno 118 | Network unreachable | Check WiFi connection and server IP |

---

## RTMP Debug Journal

See [RTMP Debug Journal](docs/rtmp_reference/RTMP_DEBUG_JOURNAL.md) for detailed history of RTMP debugging efforts, including:
- Chunk stream ID fixes (connect=3, createStream=3, publish=4, video=4)
- FLV-wrapped vs raw NAL video format
- Client-side control messages per ffmpeg
- Stream ID handling (0 for video, 1 for publish)
- Server-assigned stream ID from createStream response
- tcUrl format matching ffmpeg (user:pass@host:port/app/stream)
- onMetaData before first video frame

---

## Reference Documentation

- **RTMP Reference:** `docs/rtmp_reference/` — ffmpeg RTMP source, MediaMTX RTMP analysis, RTMP debug journal
- **SRT Reference:** `docs/srt_reference/` — MediaMTX SRT source (conn.go, server.go, dest.go, streamid.go, listener.go)

### Planning & Diagnostic Documents
- **Remote Validation Plan:** `REMOTE_VALIDATION_PLAN.md` — step-by-step validation against production MediaMTX (172.160.27.93)
- **RTMP Diagnostic Plan:** `RTMP_DIAGNOSTIC_PLAN.md` — root cause analysis of errno 118 (ETIMEDOUT) and diagnostic stages
- **RTMP E2E Test Plan:** `RTMP_E2E_TEST_PLAN.md` — end-to-end test plan (network, build/flash, serial stages, MediaMTX verification)
- **Glossary:** `docs/reference/GLOSSARY.md` — STANAG/MISB, video, sensor, ESP32, networking terminology
- **SRT Setup Guide:** `docs/SRT_SETUP.md` — enabling SRT transport, MediaMTX config, ffmpeg testing
- **MediaMTX RTMP Reference:** `docs/rtmp_reference/MEDIAMTX_RTMP.md` — MediaMTX RTMP source files, handshake, chunk IDs, FLV video format
- **MediaMTX SRT Reference:** `docs/rtmp_reference/MEDIAMTX_SRT.md` — MediaMTX SRT source files, handshake, DATA packet format, socket IDs

### Core Documentation Index
- `README.md` — project entry point, features, quick start, structure
- `docs/architecture/OVERVIEW.md` — architecture diagrams, partitions, build artifacts
- `docs/architecture/DATA-FLOW.md` — per-frame 30 fps pipeline and timing budget
- `docs/architecture/MEDIAMTX.md` — ground station, ports, WebRTC, Quest, KLV extraction
- `docs/build/BUILD.md` — build environment, steps, artifacts, troubleshooting
- `docs/testing/TESTING.md` — 64 Unity tests, QEMU run, coverage
- `docs/testing/TROUBLESHOOTING.md` — symptom→cause→fix decision trees
- `docs/reference/API.md` — complete API reference (update on signature changes)
- `docs/reference/QUICK-REFERENCE.md` — commands, ports, tags, PIDs cheat sheet

---

## Key Conventions

- **Static buffers** for hot path (no malloc in 30fps loop)
- **PSRAM** for large allocations (`MALLOC_CAP_SPIRAM`)
- **ESP_LOGI(TAG, ...)** with module-specific TAG
- **Error handling:** `ESP_ERROR_CHECK()` for init, check returns in loop
- **Tests:** Unity, `setUp()`/`tearDown()`, `RUN_TEST()` in `test_runner.c`
- **Docs:** Update `docs/reference/API.md` when changing function signatures

---

## C LSP (clangd) Setup

A clangd-based C Language Server is configured for IDE-like features (completion, go-to-definition, diagnostics).

**Binary:** `~/.local/bin/clangd` (v22.1.1)

**Configuration:** `/workspaces/drone/.clangd` — reads `build/compile_commands.json` for include paths and defines.

**How to use:**

```bash
# 1. Build first (generates compile_commands.json)
cd /workspaces/drone/firmware/stanag-s3-simulator
idf.py build

# 2. Add clangd to PATH
export PATH="$HOME/.local/bin:$PATH"

# 3. Verify
clangd --version
```

**VS Code:** The devcontainer recommends the `llvm-vs-code-extensions.vscode-clangd` extension. clangd is auto-installed via pip in `.devcontainer/setup-esp-idf.sh` and added to PATH in `esp-idf-env.sh`.

**Key files:**
- `/workspaces/drone/.clangd` — clangd configuration
- `/workspaces/drone/.vscode/settings.json` — VS Code integration settings
- `build/compile_commands.json` — generated by CMake, used by clangd for indexing

---

## When in Doubt

1. **Read `main/stanag_types.h`** — all types in one place
2. **Check `test/test_runner.c`** — shows module init/usage patterns
3. **Run tests on QEMU** — validates without hardware
4. **Parse serial output** — `python3 scripts/parse_ts_output.py serial.log`
5. **Check `docs/architecture/DATA-FLOW.md`** — Mermaid sequence diagram of pipeline