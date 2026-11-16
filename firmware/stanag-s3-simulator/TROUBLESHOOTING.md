# Troubleshooting Guide — STANAG S3 Simulator

## Quick Decision Trees

### Build Failures

```
Build fails?
├── "Failed to create SPIFFS image for partition 'spiffs'"?
│   └── Fix: Add CONFIG_PARTITION_TABLE_CUSTOM=y to sdkconfig.defaults
├── "undefined reference to esp_psram_get_size"?
│   └── Fix: Add CONFIG_SPIRAM=y to sdkconfig.defaults
├── "idf.py: command not found"?
│   └── Fix: Run `eim shell v5.5.6` in devcontainer
├── "partition table doesn't match"?
│   └── Fix: Check partitions.csv matches sdkconfig (offset/size)
└── "SPIFFS partition too small"?
    └── Fix: Increase spiffs partition size in partitions.csv (0xEF000 → larger)
```

### Runtime Issues (Serial Monitor)

```
No output on /dev/ttyACM0?
├── Check device exists: ls -la /dev/ttyACM0
├── Check baud rate: stty -F /dev/ttyACM0 115200 raw -echo
├── Try: cat /dev/ttyACM0 (may need sudo/dialout group)
├── Check dmesg: dmesg -T | grep -i ttyACM
└── Try different USB cable/port

Garbled output?
├── Wrong baud rate → Must be 115200 8N1
├── Flow control → Disable: stty -F /dev/ttyACM0 -crtscts
├── Encoding issue → Use `cat -v` to see control chars
└── Buffer overrun → Increase terminal buffer or log to file

"Frame overrun: X ms (budget 33 ms)"?
├── Expected occasionally in QEMU/WSL
├── Profile: Add ESP_LOGI at each pipeline stage
├── Reduce frame rate: Change FRAME_RATE_HZ in main.c
├── Optimize: Check for malloc in hot path (use static buffers)
└── PSRAM speed: Try CONFIG_SPIRAM_SPEED_40M=y (more stable)

"UDP send failed: errno 118"?
├── Expected in WSL2/QEMU (no network stack)
├── Fix for local testing: network_tx_set_unix_socket("/tmp/stanag.sock")
├── Configure MediaMTX: source: unix+mpegts:///tmp/stanag.sock
├── Hardware: Check WiFi connected, correct MediaMTX IP
└── Firewall: Allow UDP 8888 on ground station
```

### Sensor/Simulation Issues

```
GPS shows 0,0 or fixed position?
├── Check sensor_sim_get_data() frame_num increments
├── Verify circular path math in sensor_sim.c
├── Check GPS fix_type = 4 (RTK Fixed) in output
└── Confirm 500m radius, Aarhus center (56.1735, 10.2040)

IMU values seem wrong?
├── Banked turn: roll should vary ±10° in turns
├── Heading should match GPS heading
├── Accel Z should be ~1g (gravity) + centripetal
└── Quaternion should be normalized (w²+x²+y²+z²=1)

KLV parsing fails?
├── Check PID 0x0101 packets in TS_OUT
├── Verify KLV key/length big-endian encoding
├── Check parse_klv_value() in parse_ts_output.py matches builder
└── Ensure 2-byte key, 2-byte length, then value
```

### MediaMTX / WebRTC Issues

```
No video in browser?
├── Check MediaMTX logs: "source 'udp' connected"
├── Verify ESP32 sending to correct IP:port (192.168.4.2:8888)
├── Check firewall: UDP 8888 allowed on MediaMTX host
├── Try: curl http://media_mtx:9997/v3/paths/list
└── Check MediaMTX config: sourceFormat: mpegts

WebRTC connection fails?
├── Browser requires HTTPS for WebRTC (or Chrome insecure flag)
├── Chrome: chrome://flags/#unsafely-treat-insecure-origin-as-secure
├── Add: http://<media_mtx_ip>:8889
├── Check STUN servers reachable (Google STUN default)
├── For production: Configure TURN server
└── Quest: Use Firefox Reality if Chrome fails

High latency (>500ms)?
├── Use UDP transport: webrtcTransport: udp
├── Reduce GOP: More frequent IDR frames (lower latency, lower quality)
├── Local network only: WiFi 5/6 AP <5ms, avoid 4G
├── Check MediaMTX jitter buffer settings
└── Browser: Use WebCodecs API (hardware decode)
```

### Flash/Hardware Issues

```
Flash fails?
├── Check device: ls /dev/ttyUSB* /dev/ttyACM*
├── Hold BOOT button while connecting for download mode
├── Try lower baud: idf.py -p /dev/ttyUSB0 -b 460800 flash
├── Erase flash first: idf.py -p /dev/ttyUSB0 erase-flash
└── Check USB cable (data + power, not charge-only)

Boot loop / Guru Meditation?
├── Check ESP_LOG output for crash reason
├── Common: Stack overflow → Increase task stack size
├── Common: PSRAM not initialized → CONFIG_SPIRAM_BOOT_INIT=y
├── Common: Watchdog → esp_task_wdt_deinit() in main.c (done)
└── Use: idf.py monitor for panic decode

Brownout / Reset under load?
├── Power supply insufficient → Use 3A+ 3.3V regulator
├── Add bulk capacitance (470µF) on 3.3V rail
├── Check PSRAM power: CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL
└── Reduce WiFi TX power if not needed
```

---

## Common Error Messages

| Error | Location | Meaning | Fix |
|-------|----------|---------|-----|
| `E (123) SPIFFS: mount failed` | Boot | SPIFFS partition not found | `CONFIG_PARTITION_TABLE_CUSTOM=y` |
| `E (456) PSRAM: not initialized` | Boot | PSRAM disabled | `CONFIG_SPIRAM=y` + `CONFIG_SPIRAM_BOOT_INIT=y` |
| `E (789) HEAP: allocation failed` | Runtime | Out of memory | Use PSRAM (`MALLOC_CAP_SPIRAM`) |
| `W (1000) STANAG_POC: Frame overrun` | Runtime | >33ms frame time | Profile, optimize, or reduce fps |
| `W (1001) NETWORK_TX: UDP send failed: errno 118` | Runtime | Network unreachable | Expected in WSL; use Unix socket |
| `E (1002) JPEG_LOAD: Failed to alloc` | Runtime | PSRAM alloc failed | Check PSRAM size, reduce buffer |
| `Guru Meditation: Core 0 panic` | Crash | Fatal exception | Check backtrace in monitor |

---

## Debugging Techniques

### 1. Enable Verbose Logging
```c
// In main.c or component
esp_log_level_set("STANAG_POC", ESP_LOG_DEBUG);
esp_log_level_set("JPEG_LOAD", ESP_LOG_DEBUG);
esp_log_level_set("TS_MUXER", ESP_LOG_DEBUG);
```

### 2. Profile Frame Timing
```c
// In main.c loop
uint32_t t0 = esp_timer_get_time();
// ... pipeline stage ...
uint32_t t1 = esp_timer_get_time();
ESP_LOGI(TAG, "Stage took %lu us", t1 - t0);
```

### 3. Memory Analysis
```bash
# Build with size info
idf.py size-components

# Heap tracing (menuconfig)
# Component config → Heap memory debugging → Enable heap tracing
```

### 4. Serial Capture for Analysis
```bash
# Capture with timestamps
cat /dev/ttyACM0 | ts '[%Y-%m-%d %H:%M:%.S]' > serial.log

# Parse later
python3 scripts/parse_ts_output.py serial.log
```

### 5. QEMU Debugging
```bash
# Run with GDB
idf.py -p /dev/null qemu monitor &
# In another terminal:
riscv32-esp-elf-gdb build/stanag-s3-simulator.elf
(gdb) target remote :3333
(gdb) break main.c:210
(gdb) continue
```

---

## Configuration Checklist

### Required sdkconfig.defaults
- [ ] `CONFIG_PARTITION_TABLE_CUSTOM=y`
- [ ] `CONFIG_SPIRAM=y`
- [ ] `CONFIG_SPIRAM_BOOT_INIT=y`
- [ ] `CONFIG_SPIRAM_MODE_OCT=y`
- [ ] `CONFIG_SPIRAM_SPEED_40M=y`
- [ ] `CONFIG_ESP_MAIN_TASK_STACK_SIZE=12288` (or larger)

### Required partitions.csv
- [ ] `nvs` at 0x9000, 0x6000
- [ ] `phy_init` at 0xF000, 0x1000
- [ ] `factory` at 0x10000, 0x100000 (1 MB)
- [ ] `spiffs` at 0x110000, 0xEF000 (~960 KB)

### Hardware Connections
- [ ] ESP32-S3 DevKit with PSRAM
- [ ] USB cable (data + power)
- [ ] `/dev/ttyACM0` or `/dev/ttyUSB0` appears
- [ ] MediaMTX reachable on UDP 8888 (for network TX)

---

## Getting Help

1. **Check this guide** — Most issues covered above
2. **Read component docs** — `firmware/stanag-s3-simulator/*.md`
3. **Run tests** — `idf.py -DTEST_COMPONENT=1 build && timeout 60 idf.py -p /dev/null qemu monitor`
4. **Parse serial output** — `python3 scripts/parse_ts_output.py serial.log`
5. **Check ESP-IDF docs** — https://docs.espressif.com/projects/esp-idf/en/v5.5.6/
6. **Search issues** — GitHub/Codeberg for similar problems

---

## Useful Debug Commands

```bash
# Device info
ls -la /dev/ttyACM*
dmesg -T | grep -i -e tty -e usb

# Serial with timestamps
cat /dev/ttyACM0 | ts '[%H:%M:%.S]'

# Monitor with IDF (auto-baud, colors)
idf.py -p /dev/ttyACM0 monitor

# Build with verbose
idf.py -v build

# Check partition table
idf.py partition-table

# Erase and reflash
idf.py -p /dev/ttyUSB0 erase-flash flash monitor
```