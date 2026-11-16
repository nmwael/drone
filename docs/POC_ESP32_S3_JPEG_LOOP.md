# ESP32-S3 POC: JPEG Loop + Simulated Sensors + STANAG 4609 Stream

## Objective
Test the complete ESP32-S3 vision pipeline **without physical cameras/sensors**:
- Loop 5 prepackaged JPEGs as "camera frames" (day scenes only)
- Simulate GPS (position, velocity, time)
- Simulate IMU (accel, gyro, orientation)
- Simulate Magnetometer (heading)
- Simulate Barometer (altitude, pressure)
- Simulate AHT20 (temp, humidity)
- Encode JPEGs → H.264 → MPEG-TS + KLV metadata
- Stream via ESP-NOW/UDP to ground station
- Output telemetry via UART to ESP32-C3 (or serial for standalone test)

**Scope:** Day camera only — **No thermal, no night vision, no AI inference**

---

## Hardware Required
- ESP32-S3 DevKit (with PSRAM, 8MB+)
- USB cable (power + serial)
- Optional: ESP32-C3 for UART receive test

---

## Software Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 POC                             │
│  ┌─────────────┐  ┌─────────────┐  ┌────────────────────────┐  │
│  │ JPEG Store  │  │ Sim Sensors │  │   H.264 Encoder        │  │
│  │ (5 JPEGs    │  │ (GPS, IMU,  │  │   (HW accelerated)     │  │
│  │  in SPIFFS) │  │  Mag, Baro, │  │                        │  │
│  └──────┬──────┘  │  Temp/RH)   │  └───────────┬────────────┘  │
│         │         └──────┬──────┘               │              │
│         ▼               ▼                      ▼              │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │              Frame Composer + KLV Builder               │  │
│  │  - JPEG → YUV420 → H.264 frame                          │  │
│  │  - Sim sensor data → MISB ST 0601 KLV packets           │  │
│  │  - Timestamp sync (sim GPS PPS)                         │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │                                 │
│                             ▼                                 │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │              MPEG-TS Muxer (188-byte packets)           │  │
│  │  - PID 0x100: Video (H.264)                             │  │
│  │  - PID 0x101: KLV Metadata                              │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │                                 │
│              ┌──────────────┴──────────────┐                 │
│              ▼                             ▼                 │
│  ┌─────────────────────┐       ┌─────────────────────┐      │
│  │ ESP-NOW / UDP TX    │       │ UART → ESP32-C3     │      │
│  │ (Ground Station)    │       │ (Telemetry + KLV)   │      │
│  └─────────────────────┘       └─────────────────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

---

## JPEG Preparation

### 5 Test Images (store in `data/` folder, upload via SPIFFS)
| File | Description | Suggested Content |
|------|-------------|-------------------|
| `frame_0.jpg` | Day scene 0 | Straight road, clear sky |
| `frame_1.jpg` | Day scene 1 | Gentle left curve |
| `frame_2.jpg` | Day scene 2 | Gentle right curve |
| `frame_3.jpg` | Day scene 3 | Straight with distant trees |
| `frame_4.jpg` | Day scene 4 | Intersection approach |

### Specs
- Resolution: **1280×720** (720p) — matches STANAG/H.264 config
- Quality: 85-90% JPEG
- Color: YUV420 (matches H.264 encoder input)
- Size: ~50-100 KB each

### Upload to SPIFFS
```bash
# In firmware/esp32-s3-vision/
idf.py spiffs-image data/spiffs.img data/
idf.py flash-spiffs
```

---

## Simulated Sensor Data

### GPS (u-blox ZED-F9P simulation)
```c
typedef struct {
    uint32_t week;           // GPS week number
    uint32_t tow_ms;         // Time of week (ms)
    int32_t lat_deg7;        // Latitude × 1e7 (deg)
    int32_t lon_deg7;        // Longitude × 1e7 (deg)
    int32_t height_mm;       // Ellipsoid height (mm)
    int32_t h_msl_mm;        // MSL height (mm)
    uint32_t h_acc_mm;       // Horizontal accuracy (mm)
    uint32_t v_acc_mm;       // Vertical accuracy (mm)
    int32_t vel_n_mm;        // Velocity North (mm/s)
    int32_t vel_e_mm;        // Velocity East (mm/s)
    int32_t vel_d_mm;        // Velocity Down (mm/s)
    uint32_t speed_mm;       // Speed (mm/s)
    int32_t heading_deg5;    // Heading × 1e5 (deg)
    uint16_t p_dop;          // Position DOP × 0.01
    uint8_t fix_type;        // 0=none, 2=2D, 3=3D, 4=RTK fixed
    uint8_t flags;           // GNSS fix OK, diff soln, etc.
    uint8_t num_sv;          // Number of satellites
} gps_sim_t;

// Simulate: circular path ~500m radius, RTK fixed
```

### IMU (ICM-20948 simulation)
```c
typedef struct {
    int16_t accel_x;  // mg (milli-g)
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;   // mdps (milli-deg/s)
    int16_t gyro_y;
    int16_t gyro_z;
    float quat_w, quat_x, quat_y, quat_z;  // Orientation quaternion
    float roll, pitch, yaw;                 // Euler angles (deg)
} imu_sim_t;

// Simulate: gentle vibration (1-2 Hz), banked turns on curves
```

### Magnetometer (QMC5883L simulation)
```c
typedef struct {
    int16_t mag_x;  // μT × 100
    int16_t mag_y;
    int16_t mag_z;
    float heading_deg;  // 0-360
} mag_sim_t;

// Simulate: Earth field ~50 μT, heading from GPS track
```

### Barometer (BMP280 simulation)
```c
typedef struct {
    int32_t pressure_pa;  // Pa
    int32_t altitude_m;   // m × 100
    int16_t temp_c;       // °C × 100
} baro_sim_t;

// Simulate: 1013 hPa, altitude from GPS + noise
```

### AHT20 (Temp/Humidity simulation)
```c
typedef struct {
    int16_t temp_c;    // °C × 100
    int16_t humidity;  // %RH × 100
} aht20_sim_t;

// Simulate: 20°C, 50% RH + slow drift
```

---

## KLV Metadata (MISB ST 0601)

### Per-Frame KLV Tags
| Tag | Name | Source | Value |
|-----|------|--------|-------|
| 0x0001 | Position (Lat/Lon/Height) | Sim GPS | deg7/deg7/mm |
| 0x0002 | Velocity NED | Sim GPS | mm/s |
| 0x0003 | Platform Heading | Sim Mag | deg5 |
| 0x0102 | Barometric Altitude | Sim Baro | mm |
| 0x0103 | Platform Temperature | Sim AHT20 | °C × 10 |
| 0x0104 | Magnetic Heading | Sim Mag | deg5 |
| 0xFFFF | Timestamp | Sim GPS PPS | µs |

**Matches STANAG S3 Simulator KLV tag set**

---

## Implementation Tasks

### 1. JPEG Loader (`jpeg_loader.c`)
- Mount SPIFFS
- Load 5 JPEGs into PSRAM buffers at startup
- Provide `get_next_frame()` → returns YUV420 buffer (via SW decode stub)

### 2. Sensor Simulators (`sensor_sim.c`)
- `gps_sim_update()` — increment position on circular path
- `imu_sim_update()` — vibration + orientation (banked turns)
- `mag_sim_update()` — heading from GPS track
- `baro_sim_update()` — altitude + pressure with lapse rate
- `aht20_sim_update()` — slow temp/RH drift

### 3. KLV Builder (`klv_builder.c`)
- Build MISB ST 0601 local set per frame
- Use simulated sensor data
- Inject into TS packet PID 0x0101

### 4. H.264 Encoder (`h264_encoder.c`)
- Config: 1280×720 @ 30fps, 4 Mbps CBR, High Profile L4.1
- **HW encoder** (ESP32-S3 ENC peripheral) for production
- SW fallback for initial testing (matches simulator)

### 5. TS Muxer (`ts_muxer.c`)
- PID 0x0000: PAT
- PID 0x1000: PMT
- PID 0x0100: Video (H.264)
- PID 0x0101: KLV (per frame)
- PID 0x1FFF: Null packets
- PCR on video PID

### 6. Network TX (`network_tx.c`)
- ESP-NOW to peer OR UDP to ground IP
- Frame rate control (33ms interval)
- Stats logging (bitrate, frame drops)

### 7. UART Telemetry (`uart_telemetry.c`)
- JSON format to ESP32-C3
- Include: GPS, IMU, Mag, Baro, AHT20, frame stats

### 8. Main Loop (`main.c`)
```c
void app_main() {
    init_spiffs();           // Load JPEGs
    init_sensor_sims();      // Init all simulators
    init_h264_encoder();     // HW encoder (or SW fallback)
    init_klv_builder();      // KLV
    init_ts_muxer();         // TS
    init_network_tx();       // ESP-NOW/UDP
    init_uart_telemetry();   // UART to C3

    while (1) {
        frame = get_next_jpeg();           // 0-4 loop
        yuv = jpeg_to_yuv420(frame);       // SW decode (POC) / HW ISP (prod)
        h264 = h264_encode(yuv);           // HW encode
        klv = klv_build(sim_sensors);      // KLV packet
        ts = ts_mux(h264, klv);            // TS packets
        network_tx(ts);                    // ESP-NOW/UDP
        uart_tx(sim_sensors, frame_stats); // UART to C3
        sim_sensors_update();              // Increment sims
        vTaskDelay(33);                    // 30 fps
    }
}
```

---

## Build & Flash

```bash
cd firmware/esp32-s3-vision
idf.py set-target esp32s3
idf.py menuconfig  # Enable PSRAM, SPIFFS, H.264 HW encoder
idf.py build
idf.py flash monitor
```

---

## Expected Output

### Serial Monitor (115200 baud)
```
I (123) POC: Loaded 5 JPEGs from SPIFFS (total 384 KB)
I (145) POC: Sensor sims initialized
I (167) POC: H.264 encoder ready (1280x720 @ 30fps, 4 Mbps)
I (189) POC: ESP-NOW peer added (C3 MAC: xx:xx:xx:xx:xx:xx)
I (211) POC: Starting main loop @ 30 fps

I (3333) POC: Frame 0 | GPS: 55.123456, 9.654321 | Alt: 52.3m | Hdg: 45.2° | Temp: 20.1°C
I (3366) POC: Frame 1 | Encoded: 12.4 KB | Bitrate: 3.8 Mbps | KLV: 247 bytes
I (3399) POC: Frame 2 | TX: ESP-NOW OK | UART: 187 bytes
...
```

### Ground Station (MediaMTX + WebRTC)
- Receives MPEG-TS on UDP:8888
- Decodes H.264 → displays 5 looping day images
- KLV extracted → telemetry overlay shows simulated data

### ESP32-C3 UART Receive
```json
{
  "frame": 0,
  "gps": {"lat": 55.123456, "lon": 9.654321, "alt": 52.3, "hdg": 45.2, "fix": 4, "sats": 12},
  "imu": {"roll": 0.5, "pitch": -0.2, "yaw": 45.2},
  "mag": {"heading": 45.2},
  "baro": {"pressure": 101325, "alt": 52.3},
  "aht20": {"temp": 20.1, "rh": 50.2},
  "stats": {"frame_size": 12400, "bitrate": 3800000, "klv_size": 247}
}
```

---

## Next Steps After POC Works

1. **Add real OV5640 camera** — replace JPEG loader with CSI capture
2. **Connect real GPS/IMU/Mag/Baro/AHT20** — replace simulators
3. **Connect ESP32-C3** — full sensor fusion EKF

**No thermal, no night vision, no RP2040 ToF, no AI inference**

---

## Files to Create

```
firmware/esp32-s3-vision/
├── data/
│   ├── frame_0.jpg
│   ├── frame_1.jpg
│   ├── frame_2.jpg
│   ├── frame_3.jpg
│   ├── frame_4.jpg
│   └── CMakeLists.txt          # SPIFFS image generation
├── main/
│   ├── main.c                  # POC entry point
│   ├── jpeg_loader.c/.h        # SPIFFS JPEG load + decode
│   ├── sensor_sim.c/.h         # All sensor simulators
│   ├── h264_encoder.c/.h       # HW H.264 encode
│   ├── klv_builder.c/.h        # MISB ST 0601 KLV
│   ├── ts_muxer.c/.h           # MPEG-TS mux
│   ├── network_tx.c/.h         # ESP-NOW/UDP TX
│   ├── uart_telemetry.c/.h     # UART to C3
│   └── CMakeLists.txt
├── sdkconfig.defaults          # PSRAM, SPIFFS, H.264 HW encoder
└── partitions.csv              # SPIFFS partition
```

---

## Related Documentation

| Document | Location |
|----------|----------|
| **AI Onboarding (Vision)** | `../firmware/esp32-s3-vision/AGENTS.md` |
| **Quick Reference (Vision)** | `../firmware/esp32-s3-vision/QUICK-REFERENCE.md` |
| **AI Onboarding (C3 Hub)** | `../firmware/esp32-c3-hub/AGENTS.md` |
| **STANAG S3 Simulator** | `../firmware/stanag-s3-simulator/` (reference implementation) |
| **MISB ST 0601** | https://www.gwg.nga.mil/misb/docs/ST0601.pdf |
| **STANAG 4609** | https://www.nato.int/stanag/ |