# AI Agent Instructions — Drone Monorepo

## Quick Context

**This is a monorepo** for an RC monster truck autonomy project with STANAG 4609 video streaming capability.

### Currently Active & Running
| Component | Path | Status |
|-----------|------|--------|
| **STANAG S3 Simulator** | `firmware/stanag-s3-simulator/` | ✅ **Flashing on `/dev/ttyACM0`** |
| MediaMTX Config | `ground-station/mediamtx/` | ✅ Ready |
| WebXR App Scaffold | `ground-station/webxr-app/` | 🚧 Early |

### Planned Subsystems
- `firmware/esp32-s3-vision/` — Production vision pipeline
- `firmware/esp32-c3-hub/` — Sensor fusion hub
- `firmware/rp2040-tof-{1,2,3}/` — ToF sensor arrays

---

## STANAG S3 Simulator — Key Facts for AI

**Target:** ESP32-S3 with PSRAM (8MB+)  
**Framework:** ESP-IDF v5.5.6  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

### Architecture (30 fps pipeline)
```
SPIFFS (5 JPEG) → JPEG Loader → YUV420 (dummy) → H.264 Encoder → NAL Units
                                                              ↓
Sensor Simulator → GPS/IMU/Mag/Baro/AHT20 → KLV Builder → MISB ST 0601 KLV
                                                              ↓
                                                    MPEG-TS Muxer → TS Packets (188B)
                                                              ↓
                                        Network TX (UDP/ESP-NOW) → MediaMTX:8888
                                                              ↓
                                                    UART Telemetry → JSON @ 115200
```

### Critical Data Structures (`main/stanag_types.h`)
```c
sensor_data_t  // Aggregated: gps, imu, mag, baro, aht20
jpeg_frame_t   // SPIFFS JPEG + metadata
yuv420_frame_t // YUV420 planar (1280x720)
h264_nalu_t    // NAL units (SPS/PPS/IDR/AUD)
klv_packet_t   // KLV-encoded bytes
ts_packet_t    // MPEG-TS packets (count × 188B)
```

### KLV Tags (MISB ST 0601 UAS Local Set)
| Key | Name | Format |
|-----|------|--------|
| 0x0001 | Position | deg7/deg7/mm (int32×3) |
| 0x0002 | Velocity NED | mm/s (int32×3) |
| 0x0003 | Platform Heading | deg5 (int32) |
| 0x0102 | Barometric Altitude | mm (int32) |
| 0x0103 | Temperature | int16 × 0.1°C |
| 0x0104 | Magnetic Heading | deg5 (int32) |
| 0xFFFF | Timestamp | µs (uint32) |

### MPEG-TS PIDs
| PID | Stream | Content |
|-----|--------|---------|
| 0x0000 | PAT | Program Association Table |
| 0x1000 | PMT | Program Map Table |
| 0x0100 | Video | H.264 PES + PCR |
| 0x0101 | KLV | Private stream (MISB) |
| 0x1FFF | Null | Padding |

---

## Common AI Tasks & Patterns

### 1. Reading Live Telemetry
```bash
# Raw serial (115200 8N1)
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0

# With parsing
python3 firmware/stanag-s3-simulator/scripts/parse_ts_output.py serial.log
```

### 2. Building & Testing
```bash
# In devcontainer (ESP-IDF pre-sourced via EIM)
cd firmware/stanag-s3-simulator

# Build main app
idf.py build

# Build & run tests on QEMU
idf.py -DTEST_COMPONENT=1 build
timeout 60 idf.py -p /dev/null qemu monitor

# Flash hardware
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. Adding a New KLV Tag
1. Add key to `KLV_KEYS` in `main/klv_builder.c`
2. Add encoding logic in `klv_builder_build()`
3. Update `parse_klv_value()` in `scripts/parse_ts_output.py`
4. Add test in `test/klv_builder_test.c`
5. Update `API.md` and `DATA-FLOW.md` tables

### 4. Modifying Sensor Simulation
- Edit `main/sensor_sim.c` → `sensor_sim_get_data()`
- Update `sensor_data_t` in `main/stanag_types.h` if new fields needed
- Run tests: `idf.py -DTEST_COMPONENT=1 build && timeout 60 idf.py -p /dev/null qemu monitor`

### 5. Changing Frame Rate
- Modify `FRAME_RATE_HZ` in `main/main.c`
- Update timing budget in `DATA-FLOW.md` (Mermaid gantt)
- Adjust `FRAME_INTERVAL_MS` calculation

---

## Repository Navigation for AI

### Root-Level Docs
| File | Purpose |
|------|---------|
| `README.md` | Project overview, subsystem index, quick links |
| `AGENTS.md` | This file — AI onboarding |
| `docs/README.md` | Documentation index by category |
| `docs/DEV_CONTAINER.md` | Dev container setup, USB passthrough |

### STANAG S3 Simulator Docs
| File | Purpose |
|------|---------|
| `firmware/stanag-s3-simulator/README.md` | Quick start, features, hardware |
| `firmware/stanag-s3-simulator/OVERVIEW.md` | Architecture + Mermaid diagrams |
| `firmware/stanag-s3-simulator/DATA-FLOW.md` | Per-frame pipeline + timing |
| `firmware/stanag-s3-simulator/API.md` | Complete API reference |
| `firmware/stanag-s3-simulator/BUILD.md` | Build guide + troubleshooting |
| `firmware/stanag-s3-simulator/TESTING.md` | Unit tests (64), QEMU, CI |
| `firmware/stanag-s3-simulator/MEDIAMTX.md` | Ground station + WebRTC + Quest |

### Key Source Files
| File | Purpose |
|------|---------|
| `firmware/stanag-s3-simulator/main/main.c` | Entry point, 30fps main loop |
| `firmware/stanag-s3-simulator/main/stanag_types.h` | All data structures |
| `firmware/stanag-s3-simulator/main/jpeg_loader.c/h` | SPIFFS JPEG → YUV |
| `firmware/stanag-s3-simulator/main/sensor_sim.c/h` | Simulated sensors |
| `firmware/stanag-s3-simulator/main/h264_encoder.c/h` | SW H.264 fallback |
| `firmware/stanag-s3-simulator/main/klv_builder.c/h` | MISB ST 0601 KLV |
| `firmware/stanag-s3-simulator/main/ts_muxer.c/h` | MPEG-TS muxer |
| `firmware/stanag-s3-simulator/main/network_tx.c/h` | UDP/ESP-NOW TX |
| `firmware/stanag-s3-simulator/main/uart_telemetry.c/h` | UART JSON output |

---

## Troubleshooting Quick Reference

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| `SPIFFS image generation fails` | Missing partition config | `CONFIG_PARTITION_TABLE_CUSTOM=y` in `sdkconfig.defaults` |
| `undefined reference to esp_psram_*` | PSRAM not enabled | `CONFIG_SPIRAM=y` in `sdkconfig.defaults` |
| `idf.py: command not found` | ESP-IDF not sourced | Run `eim shell v5.5.6` in devcontainer |
| `UDP send failed: errno 118` | Network unreachable (WSL/QEMU) | Expected — use Unix socket: `network_tx_set_unix_socket("/tmp/stanag.sock")` |
| `Frame overrun: X ms (budget 33 ms)` | Pipeline too slow | Profile with `ESP_LOGI` at frame boundaries; reduce frame rate or optimize |
| `JPEG decode failed` | ESP32-S3 v5.5.6 no HW JPEG | Expected — uses dummy YUV test pattern |

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

## Standards & Conventions

- **Code style:** ESP-IDF default (clang-format via `idf.py format`)
- **Logging:** `ESP_LOGI(TAG, ...)` with `TAG = "MODULE_NAME"`
- **Error handling:** Check `esp_err_t` returns, use `ESP_ERROR_CHECK()` for critical init
- **Memory:** PSRAM for large buffers (`MALLOC_CAP_SPIRAM`), static buffers for hot path
- **Tests:** Unity framework, `test/<module>_test.c`, registered in `test/test_runner.c`
- **Documentation:** Markdown with Mermaid diagrams, update `API.md` when changing APIs

---

## C LSP (clangd) Setup

A clangd-based C Language Server is configured for IDE-like features (completion, go-to-definition, diagnostics) across the monorepo.

**Binary:** `~/.local/bin/clangd` (v22.1.1)

**Configuration:** `/workspaces/drone/.clangd` — reads `compile_commands.json` from each build directory.

**How to use:**

```bash
# 1. Build the project first (generates compile_commands.json)
cd firmware/stanag-s3-simulator
idf.py build

# 2. Add clangd to PATH
export PATH="$HOME/.local/bin:$PATH"

# 3. Verify clangd works
clangd --version
```

**VS Code:** The devcontainer (`devcontainer.json`) recommends the `llvm-vs-code-extensions.vscode-clangd` extension and sets `clangd.path` to `~/.local/bin/clangd`. It auto-installs clangd via pip in `setup-esp-idf.sh`.

**Key files:**
- `.clangd` — clangd configuration (include paths, defines, compile flags)
- `.vscode/settings.json` — VS Code clangd integration settings
- `build/compile_commands.json` — generated by CMake, used by clangd for indexing

---

## Useful Commands Reference

```bash
# Devcontainer ESP-IDF
eim shell v5.5.6                    # Enter ESP-IDF v5.5.6 shell
idf.py --version                    # Verify ESP-IDF version

# STANAG S3 Simulator
cd firmware/stanag-s3-simulator
idf.py set-target esp32s3           # Set target (once)
idf.py build                        # Build
idf.py flash monitor                # Flash + monitor (hardware)
idf.py -DTEST_COMPONENT=1 build     # Build test app
timeout 60 idf.py -p /dev/null qemu monitor  # Run tests on QEMU
idf.py size-components              # Size breakdown
idf.py fullclean                    # Clean build

# Serial monitoring
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0
python3 scripts/parse_ts_output.py serial.log

# MediaMTX (ground station)
cd ground-station/mediamtx
./mediamtx mediamtx.yml             # Run with config
curl http://localhost:9997/v3/paths/list  # Check API
```

---

## When in Doubt

1. **Check the simulator's own docs first** — `firmware/stanag-s3-simulator/*.md`
2. **Read `main/stanag_types.h`** — All data structures defined there
3. **Look at `test/test_runner.c`** — Shows how modules are tested
4. **Check `scripts/parse_ts_output.py`** — Reference for KLV/TS parsing
5. **Run tests on QEMU** — Validates changes without hardware