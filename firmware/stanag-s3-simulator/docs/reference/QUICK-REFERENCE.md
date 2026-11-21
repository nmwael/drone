# Quick Reference — STANAG S3 Simulator

> **AI hint:** Cheat sheet: live serial, build/flash/test commands, ports, KLV tags, PIDs, sensor rates, frame rate. Read for fast command lookup. Related: `AGENTS.md`, `README.md`, `API.md`.

## 🎯 Live on `/dev/ttyACM0` @ 115200 baud

---

## ⚡ Essential Commands

```bash
cd /workspaces/drone/firmware/stanag-s3-simulator

# Build & Flash
idf.py set-target esp32s3           # Once
idf.py build                        # Build
idf.py -p /dev/ttyUSB0 flash monitor  # Hardware

# Test (QEMU, 64 Unity tests)
idf.py -DTEST_COMPONENT=1 build
timeout 60 idf.py -p /dev/null qemu monitor

# Size Report
idf.py size-components

# Clean
idf.py fullclean
```

---

## 📡 Serial Monitoring

```bash
# Raw output
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0

# Capture to file
cat /dev/ttyACM0 > serial.log

# Parse TS/KLV from capture
python3 scripts/parse_ts_output.py serial.log
```

---

## 📊 Key Specs

| Parameter | Value |
|-----------|-------|
| MCU | ESP32-S3 (PSRAM 8MB+) |
| Framework | ESP-IDF v5.5.6 |
| Frame Rate | 30 fps (33 ms budget) |
| Resolution | 1280×720 (5 JPEG frames) |
| Video | H.264 High Profile L4.1 (SW) |
| Container | MPEG-TS (188-byte packets) |
| Metadata | MISB ST 0601 KLV (7 tags) |
| Transport | UDP/ESP-NOW → MediaMTX:8888 |
| Telemetry | UART1 @ 115200 (JSON) |
| Sensors | GPS, IMU, Mag, Baro, AHT20 |

---

## 🏷️ KLV Tags

| Key | Name | Format | Source |
|-----|------|--------|--------|
| 0x0001 | Position | deg7/deg7/mm (int32×3) | GPS |
| 0x0002 | Velocity NED | mm/s (int32×3) | GPS |
| 0x0003 | Platform Heading | deg5 (int32) | GPS/IMU |
| 0x0102 | Baro Altitude | mm (int32) | Baro |
| 0x0103 | Temperature | int16 × 0.1°C | AHT20 |
| 0x0104 | Mag Heading | deg5 (int32) | Mag |
| 0xFFFF | Timestamp | µs (uint32) | GPS TOW |

---

## 📦 MPEG-TS PIDs

| PID | Stream | Content |
|-----|--------|---------|
| 0x0000 | PAT | Program Association Table |
| 0x1000 | PMT | Program Map Table |
| 0x0100 | Video | H.264 PES + PCR |
| 0x0101 | KLV | MISB ST 0601 (Private) |
| 0x1FFF | Null | Padding |

---

## ⏱️ Frame Budget (33 ms)

| Stage | Budget |
|-------|--------|
| JPEG load/decode | 5 ms |
| H.264 encode | 15 ms |
| Sensor sim | 2 ms |
| KLV build | 3 ms |
| TS mux | 4 ms |
| Network TX | 3 ms |
| UART telemetry | 1 ms |
| **Total** | **33 ms** |

---

## 🧪 Test Modules (64 Total)

| Module | Tests |
|--------|-------|
| KLV Builder | 8 |
| TS Muxer | 11 |
| H.264 Encoder | 9 |
| JPEG Loader | 10 |
| Sensor Sim | 14 |
| Network TX | 5 |
| UART Telemetry | 4 |

---

## 🐛 Common Fixes

| Error | Fix |
|-------|-----|
| SPIFFS image fails | `CONFIG_PARTITION_TABLE_CUSTOM=y` |
| `esp_psram_*` undefined | `CONFIG_SPIRAM=y` |
| `idf.py` not found | `eim shell v5.5.6` |
| UDP errno 118 | Expected (WSL/QEMU) — use Unix socket |
| Frame overrun | Pipeline >33ms — profile or reduce fps |
| JPEG decode fail | No HW JPEG on S3 v5.5.6 — dummy YUV |

---

## 📁 Key Files

```
main/
├── main.c              # Entry point, 30fps loop
├── stanag_types.h      # ALL data structures
├── jpeg_loader.c/h     # SPIFFS → YUV
├── sensor_sim.c/h      # Simulated sensors
├── h264_encoder.c/h    # SW H.264 fallback
├── klv_builder.c/h     # MISB ST 0601 KLV
├── ts_muxer.c/h        # MPEG-TS muxer
├── network_tx.c/h      # UDP/ESP-NOW TX
└── uart_telemetry.c/h  # UART JSON output

scripts/
├── parse_ts_output.py  # TS/KLV parser
└── klv_extractor.sh    # MediaMTX hook

test/
├── test_runner.c       # 64 Unity tests
└── *_test.c            # Per-module tests
```

---

## 🔗 Related Docs

| File | Purpose |
|------|---------|
| `README.md` | Quick start, features |
| `OVERVIEW.md` | Architecture diagrams |
| `DATA-FLOW.md` | Pipeline + timing |
| `API.md` | Complete API reference |
| `docs/build/BUILD.md` | Build guide |
| `TESTING.md` | Tests + CI |
| `MEDIAMTX.md` | Ground station |