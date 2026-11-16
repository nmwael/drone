# Quick Reference Card — Drone Monorepo

## 🎯 Currently Running: STANAG S3 Simulator
**Device:** `/dev/ttyACM0` (ESP32-S3)  
**Baud:** 115200 8N1  
**Output:** JSON telemetry + MPEG-TS hex dumps

---

## 📁 Key Paths

| Component | Path |
|-----------|------|
| STANAG S3 Simulator | `firmware/stanag-s3-simulator/` |
| ESP32-S3 Vision (planned) | `firmware/esp32-s3-vision/` |
| ESP32-C3 Hub (planned) | `firmware/esp32-c3-hub/` |
| RP2040 ToF ×3 | `firmware/rp2040-tof-{1,2,3}/` |
| MediaMTX Config | `ground-station/mediamtx/` |
| WebXR App | `ground-station/webxr-app/` |
| Documentation | `docs/` |

---

## ⚡ Essential Commands

### Devcontainer Setup
```bash
eim shell v5.5.6                    # Enter ESP-IDF v5.5.6 environment
idf.py --version                    # Verify: ESP-IDF v5.5.6
```

### STANAG S3 Simulator
```bash
cd firmware/stanag-s3-simulator

# Build & Flash
idf.py set-target esp32s3           # Once
idf.py build                        # Build
idf.py -p /dev/ttyUSB0 flash monitor  # Flash + serial (hardware)

# Testing (QEMU)
idf.py -DTEST_COMPONENT=1 build
timeout 60 idf.py -p /dev/null qemu monitor

# Size Analysis
idf.py size
idf.py size-components
idf.py size-files

# Clean
idf.py fullclean
```

### Serial Monitoring
```bash
# Raw output
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0

# Capture to file
idf.py monitor > serial.log         # Or: cat /dev/ttyACM0 > serial.log

# Parse TS/KLV from capture
python3 scripts/parse_ts_output.py serial.log
```

### MediaMTX Ground Station
```bash
cd ground-station/mediamtx
./mediamtx mediamtx.yml             # Start server

# Verify
curl http://localhost:9997/v3/paths/list
# Open browser: http://<host>:8889
```

---

## 📊 STANAG S3 Simulator — Key Specs

| Parameter | Value |
|-----------|-------|
| Target | ESP32-S3 (PSRAM required) |
| Frame Rate | 30 fps (33 ms budget) |
| Resolution | 1280×720 (5 JPEG frames) |
| Video Codec | H.264 High Profile L4.1 (SW fallback) |
| Container | MPEG-TS (188-byte packets) |
| Metadata | MISB ST 0601 KLV (UAS Local Set) |
| Transport | UDP/ESP-NOW → MediaMTX:8888 |
| Telemetry | UART1 @ 115200 (JSON) |
| Sensors Simulated | GPS, IMU, Mag, Baro, AHT20 |
| Sensor Rate | GPS/Mag/Baro: 10 Hz, IMU: 100 Hz, AHT20: 1 Hz |

---

## 🏷️ KLV Tags (MISB ST 0601)

| Key | Name | Format | Example |
|-----|------|--------|---------|
| 0x0001 | Position | deg7/deg7/mm | lat=561704250, lon=102098830, h=40000 |
| 0x0002 | Velocity NED | mm/s | vn=1234, ve=567, vd=-89 |
| 0x0003 | Platform Heading | deg5 | 13869180 (138.69°) |
| 0x0102 | Baro Altitude | mm | 40000 |
| 0x0103 | Temperature | int16×0.1°C | 206 (20.6°C) |
| 0x0104 | Mag Heading | deg5 | 13869180 |
| 0xFFFF | Timestamp | µs (uint32) | 32634394296 |

---

## 📦 MPEG-TS PIDs

| PID | Stream | Description |
|-----|--------|-------------|
| 0x0000 | PAT | Program Association Table |
| 0x1000 | PMT | Program Map Table |
| 0x0100 | Video | H.264 PES + PCR |
| 0x0101 | KLV | MISB ST 0601 (Private) |
| 0x1FFF | Null | Padding |

---

## 🔧 Frame Timing Budget (33 ms @ 30 fps)

| Stage | Budget | Notes |
|-------|--------|-------|
| JPEG load & decode | 5 ms | SPIFFS read + dummy YUV |
| H.264 encode | 15 ms | SW fallback, SPS/PPS/IDR |
| Sensor simulation | 2 ms | Circular path math |
| KLV build | 3 ms | 7 tags per frame |
| MPEG-TS mux | 4 ms | PAT/PMT/PES/Null |
| Network TX | 3 ms | UDP/ESP-NOW send |
| UART telemetry | 1 ms | JSON printf |
| **Total** | **33 ms** | Target |

---

## 🧪 Test Commands

```bash
# Run all 64 Unity tests on QEMU
cd firmware/stanag-s3-simulator
idf.py -DTEST_COMPONENT=1 build
timeout 60 idf.py -p /dev/null qemu monitor 2>&1 | tee test.log

# Check results
grep "Tests passed" test.log
grep "Tests failed" test.log

# Test modules: KLV(8) TS(11) H264(9) JPEG(10) Sensor(14) Net(5) UART(4)
```

---

## 🐛 Common Issues & Fixes

| Error | Fix |
|-------|-----|
| `SPIFFS image generation fails` | `CONFIG_PARTITION_TABLE_CUSTOM=y` in `sdkconfig.defaults` |
| `esp_psram_get_size undefined` | `CONFIG_SPIRAM=y` in `sdkconfig.defaults` |
| `idf.py not found` | `eim shell v5.5.6` |
| `UDP send failed: errno 118` | Expected in WSL/QEMU — use Unix socket |
| `Frame overrun: X ms` | Pipeline exceeds 33ms — profile or reduce frame rate |
| `JPEG decode failed` | ESP32-S3 v5.5.6 has no HW JPEG — dummy YUV used |

---

## 📚 Documentation Map

```
drone/
├── README.md              ← Project overview + subsystem index
├── AGENTS.md              ← AI onboarding (this file's parent)
├── QUICK-REFERENCE.md     ← You are here
├── docs/
│   ├── README.md          ← Doc index by category
│   └── DEV_CONTAINER.md   ← Container + USB setup
└── firmware/stanag-s3-simulator/
    ├── README.md          ← Quick start
    ├── OVERVIEW.md        ← Architecture diagrams
    ├── DATA-FLOW.md       ← Pipeline + timing
    ├── API.md             ← All APIs
    ├── BUILD.md           ← Build guide
    ├── TESTING.md         ← Tests + CI
    └── MEDIAMTX.md        ← Ground station
```

---

## 🔗 Useful Links

- **STANAG 4609:** https://www.nato.int/stanag/
- **MISB ST 0601:** https://www.gwg.nga.mil/misb/docs/ST0601.pdf
- **MediaMTX:** https://github.com/bluenviron/mediamtx
- **ESP-IDF v5.5.6:** https://docs.espressif.com/projects/esp-idf/en/v5.5.6/
- **ESP-NOW:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_now.html