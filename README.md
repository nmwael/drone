# RC Monster Truck Sensor Suite

Upgrading an RC toy monster truck with autonomous navigation sensors:
- **IR Vision** (obstacle detection, night vision)
- **Thermal Vision** (heat signature detection)
- **GPS** (positioning, waypoint navigation)
- **Compass/Magnetometer** (heading, orientation)

## Hardware Inventory

### Microcontrollers

| Module | Qty | Role | Key Features |
|--------|-----|------|--------------|
| **ESP32-C3** | 1 | Sensor Hub / Comms | RISC-V, WiFi 4/BLE 5, low power, 400KB SRAM |
| **ESP32-S3** | 1 | Vision Processing | Dual-core Xtensa LX7, WiFi/BLE, PSRAM support, AI acceleration |
| **RP2040** | 3+ | Motor Control / I/O | Dual-core Cortex-M0+, PIO for precise timing, 264KB SRAM |
| **ESP8266MOD (AI Thinker)** | 1 | **Backup Comms / Telemetry Relay** | Xtensa L106, WiFi 4 (2.4 GHz only), 160KB SRAM, 4MB Flash |

### Sensors (to procure/integrate)

| Sensor | Interface | Purpose | Candidate Parts |
|--------|-----------|---------|-----------------|
| **Thermal Camera** | I2C/SPI | Heat detection (8×8 to 32×32) | MLX90640 (32×24), AMG8833 (8×8), MLX90641 (16×12) |
| **IR Distance/Proximity** | I2C/UART | Obstacle avoidance | VL53L1X (ToF, 4m), VL53L5CX (8×8 zone), TF-Luna (LiDAR) |
| **IR Camera (Night Vision)** | CSI/Parallel | Low-light imaging | OV2640/OV5640 with IR filter removed, ESP32-S3 CSI |
| **GNSS/GPS** | UART/I2C | Positioning | u-blox ZED-F9P (RTK), BN-880, SAM-M10Q |
| **Magnetometer (Compass)** | I2C | Heading | QMC5883L, HMC5883L, RM3100, LIS3MDL |
| **IMU (Accel/Gyro)** | I2C/SPI | Orientation, vibration | ICM-20948, BMI270, MPU6050 |
| **Barometer / Pressure** | I2C | Altitude, pressure | **BMP280** (on hand), BMP390, MS5611 |
| **Temperature / Humidity** | I2C | Ambient temp & RH | **AHT20** (on hand), SHT40, BME280 |

### Power & Actuation

| Component | Specs | Notes |
|-----------|-------|-------|
| **Main Battery** | 2S/3S LiPo (7.4V/11.1V) | Power distribution board needed |
| **BEC / 5V Reg** | 5V 3A+ | For MCUs and sensors |
| **3.3V Reg** | 3.3V 1A+ | For ESP32-C3/S3, sensors |
| **Motor Driver** | Dual H-bridge, 10A+ | TB6612FNG, BTS7960, or VNH5019 |
| **Servo (Steering)** | Standard 180° | MG996R or similar |

### Salvaged Parts: Honeybee CP2 (Coaxial Helicopter)

| Part | Qty | Condition | Reuse Potential |
|------|-----|-----------|-----------------|
| **2.4GHz Receiver** | 1 | Working | Direct PWM/SBUS output → RP2040 for manual override |
| **Main Motor (Brushed)** | 1 | Working | Low-voltage test loads, winch |
| **Tail Motor (Brushed)** | 1 | Working | Auxiliary actuator |
| **Main Rotor Servos** | 2 | Working | Steering, camera gimbal (small, ~1.5kg-cm) |
| **Gyro (Piezo/MEMS)** | 1 | Working | Rate damping for truck (if analog/PWM) |
| **ESC (Brushed, built-in)** | 1 | Working | Motor driver reference / low-power loads |
| **LiPo 1S/2S Pack** | 1 | Unknown | Test power source |
| **Charger** | 1 | Working | 1S/2S LiPo charging |
| **Propellers** | 0 | **Broken** | Replace if flying again |

> **Note:** The CP2 receiver outputs standard PWM (1-2ms) or SBUS. Connect to RP2040 GPIO for RC fallback control. The coaxial mechanics (swashplate, linkages) are not directly reusable for a truck but servos are.

### ESP8266MOD (AI Thinker) — Secondary Roles

| Role | Description | Interface |
|------|-------------|-----------|
| **Telemetry Relay** | Independent low-rate telemetry uplink (MAVLink/JSON) via WiFi to ground station | UART from C3 @ 115200 → ESP8266 → UDP/TCP |
| **Watchdog / Heartbeat** | Monitors ESP32-C3/S3 health; triggers hardware reset via GPIO if stalled | GPIO → C3/S3 EN/RST pins |
| **OTA Bootstrap** | Serves firmware images for ESP32-C3/S3 OTA updates over local WiFi | SPIFFS/LittleFS + HTTP server |
| **WiFi Range Extender** | ESP-NOW → WiFi bridge for longer-range telemetry (2.4 GHz only) | ESP-NOW from C3 → ESP8266 → WiFi AP |
| **Config / Debug Portal** | Captive portal for WiFi credentials, sensor calibration, log download | Web server on ESP8266 |

> **Limitation:** ESP8266 is **single-core, no BLE, no hardware crypto, 2.4 GHz only, limited RAM**. Not suitable for video, STANAG encoding, or real-time control. Use as **auxiliary/backup only**.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      ESP32-S3 (Vision)                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Thermal Cam │  │  IR Camera  │  │   Vision Processing │  │
│  │  (MLX90640) │  │  (OV5640)   │  │   (TensorFlow Lite) │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
└─────────│────────────────│─────────────────────│────────────┘
          │                │                     │
          ▼                ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-C3 (Sensor Hub)                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   GPS/GNSS  │  │ Magnetometer│  │      IMU + Baro     │  │
│  │  (u-blox)   │  │  (QMC5883)  │  │    (ICM-20948)      │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
└─────────│────────────────│─────────────────────│────────────┘
          │                │                     │
          ▼                ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                      RP2040 ×3 (I/O)                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Motor Driver│  │   Servo     │  │  IR ToF Array       │  │
│  │  (2× H-br)  │  │  Steering   │  │  (VL53L5CX ×4)      │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │ UART (telemetry, watchdog)
                              │
                    ┌─────────┴─────────┐
                    │  ESP8266MOD       │
                    │  (AI Thinker)     │
                    │  - Telemetry UDP  │
                    │  - Watchdog RST   │
                    │  - OTA Server     │
                    └───────────────────┘
```

### Communication Bus

| Link | Protocol | Rate | Purpose |
|------|----------|------|---------|
| ESP32-S3 ↔ ESP32-C3 | ESP-NOW / UART | 1-2 Mbps | Sensor fusion, command/telemetry |
| ESP32-C3 ↔ RP2040s | UART / I2C | 1 Mbps / 400 kHz | Motor commands, sensor reads |
| RP2040 ↔ Motor Driver | PWM + GPIO | 20 kHz PWM | Motor speed/direction |
| RP2040 ↔ VL53L5CX | I2C (mux) | 1 MHz | 8×8 zone ToF scanning |
| **CP2 Receiver → RP2040** | **PWM / SBUS** | **50 Hz / 100 kbps** | **Manual RC fallback / override** |
| **ESP32-C3 → ESP8266MOD** | **UART** | **115200 baud** | **Telemetry relay, watchdog, OTA** |
| **ESP8266MOD → Ground** | **WiFi UDP/TCP** | **~1 Mbps** | **Backup telemetry, config portal** |

### Control Modes

| Mode | Source | Path | Use Case |
|------|--------|------|----------|
| **Autonomous** | ESP32-S3/C3 | ESP-NOW → UART → RP2040 | Waypoint nav, obstacle avoidance |
| **Teleop (WiFi)** | Phone/PC | WiFi → ESP32-S3 → ESP-NOW → UART → RP2040 | FPV driving, sensor streaming |
| **RC Fallback** | CP2 Transmitter | **PWM/SBUS → RP2040** | **Safety override, testing, no-WiFi zones** |

> **Safety:** RP2040 firmware implements priority: RC PWM > Autonomous > Teleop. Loss of RC signal auto-switches to autonomous hold/return.

---

## STANAG 4609 Video Encoding & Oculus Rift Teleop

### STANAG 4609 / MISB Compliance

| Requirement | Specification | Implementation |
|-------------|---------------|----------------|
| **Video Codec** | H.264 (AVC) High Profile @ Level 4.1 / MPEG-2 | ESP32-S3 hardware H.264 encoder (max 1080p@30fps) |
| **Container** | MPEG-TS (Transport Stream) | `libavformat` / custom TS muxer on S3 |
| **Metadata** | KLV (Key-Length-Value) per MISB ST 0601 | UAS Local Set, Platform Motion Imagery |
| **Coordinate System** | WGS-84, UTM, MGRS | GPS + IMU fusion from C3 hub |
| **Frame Rate** | 30 fps (NTSC) / 25 fps (PAL) | OV5640 @ 720p30 / 1080p30 |
| **Bitrate** | 2-10 Mbps (adaptive) | CBR/VBR via ESP32-S3 encoder config |
| **Latency Target** | < 200 ms glass-to-glass | Optimized pipeline (see below) |

#### KLV Metadata Packets (MISB ST 0601 UAS Local Set)

| Tag | Name | Source | Rate |
|-----|------|--------|------|
| 0x01 | Checksum | Computed | Per frame |
| 0x02 | Precision Timestamp | ESP32-S3 RTC + GPS PPS | Per frame |
| 0x03 | Mission ID | Config | Per session |
| 0x04 | Platform Tail Number | Config | Per session |
| 0x05 | Platform Heading Angle | Magnetometer (C3) | Per frame |
| 0x06 | Platform Pitch Angle | IMU (C3) | Per frame |
| 0x07 | Platform Roll Angle | IMU (C3) | Per frame |
| 0x08 | Platform True Altitude | GPS + Baro (C3) | Per frame |
| 0x09 | Platform Latitude | GPS (C3) | Per frame |
| 0x0A | Platform Longitude | GPS (C3) | Per frame |
| 0x0B | Sensor Latitude | GPS (C3) | Per frame |
| 0x0C | Sensor Longitude | GPS (C3) | Per frame |
| 0x0D | Sensor True Altitude | GPS + Baro (C3) | Per frame |
| 0x0E | Sensor Horizontal FOV | Camera calibration | Per session |
| 0x0F | Sensor Vertical FOV | Camera calibration | Per session |
| 0x10 | Frame Center Latitude | Computed | Per frame |
| 0x11 | Frame Center Longitude | Computed | Per frame |
| 0x12 | Frame Center Elevation | Computed | Per frame |
| 0x13 | Corner Lat/Lon (4 corners) | Computed | Per frame |
| 0x14 | Image Width/Height | Camera config | Per session |
| 0x15 | Frame Rate | Camera config | Per session |
| 0x2D | Sensor Ellipsoid Height | GPS + Baro | Per frame |
| 0x46 | UAS Local Set Version | Fixed (10) | Per session |
| **0x4B** | **Platform Temperature** | **AHT20 (C3)** | **Per frame** |
| **0x4C** | **Platform Relative Humidity** | **AHT20 (C3)** | **Per frame** |
| **0x4D** | **Platform Pressure** | **BMP280 (C3)** | **Per frame** |
| **0x4E** | **Platform Pressure Altitude** | **BMP280 (C3)** | **Per frame** |

> **Note:** ESP32-S3 has **hardware H.264 encoder** (max 1920×1080 @ 30fps). For STANAG compliance, use 1280×720 @ 30fps @ 4-6 Mbps CBR. KLV injection requires TS packet manipulation — do this in ESP-IDF on S3 before WiFi transmit.

---

### Oculus Rift (Meta Quest) Teleop Architecture

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           OCULUS QUEST 2/3/Pro                                │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │                    WebXR / OpenXR Runtime (Browser/App)                │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │  │
│  │  │ Left Eye    │  │ Right Eye   │  │ Head Tracking│  │ Controllers │   │  │
│  │  │ (1280×1280) │  │ (1280×1280) │  │ (6DoF, 90Hz) │  │ (6DoF each) │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘   │  │
│  └─────────│────────────────│─────────────────│────────────────│──────────┘  │
└────────────│────────────────│─────────────────│────────────────│────────────┘
             │                │                 │                │
             ▼                ▼                 ▼                ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                      GROUND STATION (PC / Raspberry Pi 5)                    │
│  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────────┐  │
│  │ WebRTC / WHIP Server│  │ STANAG TS Demux +   │  │ WebXR Teleop App    │  │
│  │ (MediaMTX / GStreamer)  │  KLV Parser         │  │ (Three.js + WebXR)  │  │
│  │  - Ingest: UDP/TCP  │  │  - Extract KLV      │  │  - Stereo render    │  │
│  │  - Transcode: H.264 │  │  - Overlay UI       │  │  - Head tracking    │  │
│  │  - Relay: WebRTC    │  │  - Record .ts/.klv  │  │  - Controller input │  │
│  └──────────┬──────────┘  └──────────┬──────────┘  └──────────┬──────────┘  │
└─────────────│────────────────────────│─────────────────────────│─────────────┘
              │                        │                         │
              ▼                        ▼                         ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                         NETWORK (WiFi 5/6 / 4G/5G)                            │
│  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────────┐  │
│  │ Video Stream        │  │ Telemetry Downlink  │  │ Control Uplink      │  │
│  │ H.264 TS @ 4-6 Mbps │  │ KLV + MAVLink @     │  │ WebRTC DataChannel  │  │
│  │ UDP / RTP           │  │ 10-50 Hz            │  │ / MQTT / ROS2       │  │
│  └─────────────────────┘  └─────────────────────┘  └─────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
              │                        │                         │
              ▼                        ▼                         ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                            ESP32-S3 (Onboard)                                 │
│  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────────┐  │
│  │ Camera Capture      │  │ H.264 Encoder       │  │ WiFi TX             │  │
│  │ OV5640 @ 720p30     │  │ HW Enc @ 4-6 Mbps   │  │ ESP-NOW / WiFi      │  │
│  │ Thermal Overlay     │  │ KLV Injection       │  │ UDP to Ground       │  │
│  └─────────────────────┘  └─────────────────────┘  └─────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### Latency Budget (Target: < 200 ms glass-to-glass)

| Stage | Typical | Target | Notes |
|-------|---------|--------|-------|
| Camera capture (OV5640) | 33 ms | 33 ms | 30 fps frame time |
| H.264 encode (HW) | 10-15 ms | 10 ms | ESP32-S3 hardware encoder |
| KLV injection + TS mux | 5 ms | 3 ms | Optimized C code |
| WiFi TX (ESP-NOW/UDP) | 5-10 ms | 5 ms | ESP-NOW preferred for low latency |
| Network (WiFi 6 / 5G) | 20-50 ms | 20 ms | Local AP < 5 ms; 4G ~30-50 ms |
| Ground station receive | 5 ms | 5 ms | UDP socket buffer |
| WebRTC decode + jitter | 30-50 ms | 20 ms | `MediaMTX` + `webrtc-unidirectional` |
| Browser decode + render | 15-20 ms | 15 ms | WebCodecs API preferred |
| **Total** | **123-188 ms** | **~111 ms** | With optimizations |

> **Critical:** Use **ESP-NOW** (not WiFi TCP) for video uplink. Ground station runs MediaMTX (Go) or GStreamer to ingest UDP → WebRTC → Browser WebXR.

---

### Video Streaming Pipeline (ESP32-S3 → Oculus)

```
ESP32-S3 Firmware (ESP-IDF):
┌─────────────────────────────────────────────────────────────────┐
│  OV5640 (CSI/Parallel)                                          │
│       │                                                         │
│       ▼                                                         │
│  ISP / DMA → YUV420 buffers (double-buffered)                   │
│       │                                                         │
│       ▼                                                         │
│  Thermal Overlay (MLX90640 → alpha blend)                       │
│       │                                                         │
│       ▼                                                         │
│  H.264 Hardware Encoder (ESP32-S3 ENC)                          │
│       │  - Profile: High @ Level 4.1                            │
│       │  - Resolution: 1280×720 @ 30fps                         │
│       │  - Bitrate: 4 Mbps CBR                                  │
│       │  - GOP: 30 (I-frame every 1s)                           │
│       ▼                                                         │
│  KLV Builder (MISB ST 0601)                                     │
│       │  - GPS/IMU from C3 via UART (10 Hz)                     │
│       │  - Timestamp from GPS PPS + RTC                         │
│       ▼                                                         │
│  MPEG-TS Muxer (188-byte packets, PID 0x100 video, 0x101 KLV)   │
│       │                                                         │
│       ▼                                                         │
│  ESP-NOW / UDP Socket → Ground Station IP:Port                  │
└─────────────────────────────────────────────────────────────────┘
```

#### Ground Station: MediaMTX Config (`mediamtx.yml`)

```yaml
paths:
  stanag:
    source: udp://0.0.0.0:8888
    sourceFormat: mpegts
    sourceOnDemand: false
    # WebRTC output for browser
    webrtc: true
    webrtcTransport: udp
    webrtcICEServers:
      - urls: stun:stun.l.google.com:19302
    # Record STANAG TS + KLV
    record: true
    recordPath: ./recordings/%path/%Y-%m-%d_%H-%M-%S.ts
    recordFormat: mpegts
    # KLV extraction hook (custom)
    runOnInit: ./scripts/klv_extractor.sh %path
```

---

### Oculus WebXR Teleop App

| Component | Technology | Repo Path |
|-----------|------------|-----------|
| **Rendering** | Three.js (WebGL2) + WebXR Layers | `ground-station/webxr-app/` |
| **Video Decode** | WebCodecs API (hardware H.264 decode) | `ground-station/webxr-app/src/video/` |
| **Stereo** | Side-by-side / Top-bottom from single stream | Shader-based split |
| **Head Tracking** | `xrFrame.getViewerPose()` → yaw/pitch/roll | Maps to camera gimbal (future) |
| **Controller Input** | `xrFrame.getPose(controller.gripSpace)` | Throttle/steering/mode |
| **Data Channel** | WebRTC DataChannel (reliable, ordered) | Commands @ 50 Hz |
| **Telemetry Overlay** | KLV parse → HUD (lat/lon/alt/heading/thermal) | Three.js CSS2DRenderer |

#### Control Mapping (Quest 2/3 Controllers)

| Input | Axis/Button | Truck Command |
|-------|-------------|---------------|
| **Left Thumbstick Y** | Axis [-1, 1] | Throttle (forward/back) |
| **Right Thumbstick X** | Axis [-1, 1] | Steering (left/right) |
| **Right Trigger** | Axis [0, 1] | Speed modifier (boost) |
| **Left Trigger** | Axis [0, 1] | Brake / reverse |
| **A Button** | Press | Mode: Autonomous |
| **B Button** | Press | Mode: Teleop |
| **X Button** | Press | Mode: RC Fallback |
| **Y Button** | Press | Thermal overlay toggle |
| **Left Grip** | Squeeze | Gimbal pitch down (future) |
| **Right Grip** | Squeeze | Gimbal pitch up (future) |
| **Head Yaw** | 6DoF pose | Camera pan (future gimbal) |
| **Head Pitch** | 6DoF pose | Camera tilt (future gimbal) |

> **Safety:** WebXR app sends `HEARTBEAT` @ 10 Hz. Loss > 500 ms → truck enters **RC Fallback** (if PWM present) or **Autonomous Hold**.

---

### Hardware Additions for STANAG + Oculus

| Item | Qty | Purpose | Notes |
|------|-----|---------|-------|
| **ESP32-S3 with PSRAM** | 1 | **Required** | H.264 encoder needs PSRAM for frame buffers (min 2MB, 8MB preferred) |
| **OV5640 Camera Module** | 1 | 720p/1080p @ 30fps | CSI/parallel; IR filter removed for night vision |
| **MLX90640 Thermal** | 1 | Thermal overlay | 32×24 @ 32 Hz, I2C |
| **u-blox ZED-F9P + GPS Antenna** | 1 | Precision timestamp + position | PPS output to ESP32-S3 GPIO for frame sync |
| **WiFi 6 Router / AP** | 1 | Ground link | ASUS RT-AX58U or similar; 5 GHz, 80 MHz channel |
| **Ground Station PC / Pi 5** | 1 | MediaMTX + WebXR host | Pi 5 can run MediaMTX; PC needed for WebXR browser |
| **Meta Quest 2/3/Pro** | 1 | HMD + Controllers | Browser: Meta Quest Browser (WebXR) or Firefox Reality |
| **External Antenna (ESP32-S3)** | 1 | Range | 2.4/5 GHz dual-band PCB antenna or u.FL → SMA |

---

### Updated Power Budget (with Video + STANAG)

| Component | Current (mA) | Voltage | Power (mW) |
|-----------|-------------|---------|------------|
| ESP32-S3 (WiFi + H.264 enc + PSRAM) | 550 | 3.3V | 1815 |
| OV5640 Camera | 180 | 3.3V | 594 |
| MLX90640 | 25 | 3.3V | 83 |
| ESP32-C3 (Sensor Hub) | 120 | 3.3V | 396 |
| RP2040 ×3 | 75 | 3.3V | 248 |
| GPS (ZED-F9P) | 65 | 3.3V | 215 |
| IMU + Mag + Baro + 4×ToF | 120 | 3.3V | 396 |
| **BMP280** | **3.6** | **3.3V** | **12** |
| **AHT20** | **2.0** | **3.3V** | **7** |
| **Total (3.3V rail)** | **~1.13A** | | **~3.77W** |
| Motor Driver + Servo (peak) | 10A+ | 7.4V | **~74W** |

> **3.3V Regulator:** Upgrade to **3A** (e.g., LM2596S-3.3 or TPS5430) with heatsink.

---

### Project Structure Updates

```
drone/
├── docs/
│   ├── hardware-inventory.md
│   ├── stanag-4609-implementation.md
│   ├── oculus-webxr-teleop.md
│   ├── wiring-diagrams/
│   ├── sensor-datasheets/
│   └── power-budget.md
├── firmware/
│   ├── esp32-s3-vision/
│   │   ├── CMakeLists.txt
│   │   ├── main/
│   │   │   ├── thermal_camera.c
│   │   │   ├── ir_camera.c
│   │   │   ├── vision_inference.c
│   │   │   ├── comms_uart.c
│   │   │   ├── h264_encoder.c          # NEW: HW H.264 encode
│   │   │   ├── klv_builder.c           # NEW: MISB ST 0601 KLV
│   │   │   ├── ts_muxer.c              # NEW: MPEG-TS packetizer
│   │   │   └── wifi_stream.c           # NEW: ESP-NOW/UDP TX
│   │   └── components/
│   ├── esp32-c3-hub/
│   │   ├── CMakeLists.txt
│   │   ├── main/
│   │   │   ├── gps_gnss.c
│   │   │   ├── magnetometer.c
│   │   │   ├── imu_baro.c
│   │   │   ├── **aht20.c**               # NEW: Temp/Humidity sensor
│   │   │   ├── **bmp280.c**              # NEW: Pressure/Temp sensor
│   │   │   ├── sensor_fusion_ekf.c
│   │   │   ├── comms_espnow.c
│   │   │   ├── comms_uart_rp2040.c
│   │   │   └── telemetry_provider.c    # NEW: KLV data source for S3
│   │   └── components/
│   ├── rp2040-motor/...
│   ├── rp2040-steering-tof/...
│   └── rp2040-rear-tof/...
├── ground-station/
│   ├── mediamtx/
│   │   ├── mediamtx.yml
│   │   └── scripts/
│   │       └── klv_extractor.sh
│   ├── webxr-app/
│   │   ├── package.json
│   │   ├── src/
│   │   │   ├── index.html
│   │   │   ├── main.ts
│   │   │   ├── video/
│   │   │   │   ├── WebCodecsDecoder.ts
│   │   │   │   ├── WebRTCReceiver.ts
│   │   │   │   └── StereoRenderer.ts
│   │   │   ├── xr/
│   │   │   │   ├── XRSessionManager.ts
│   │   │   │   ├── ControllerInput.ts
│   │   │   │   └── TeleopCommand.ts
│   │   │   ├── hud/
│   │   │   │   ├── KLVParser.ts
│   │   │   │   └── HUDOverlay.ts
│   │   │   └── comms/
│   │   │       └── DataChannel.ts
│   │   └── vite.config.ts
│   └── ros2-bridge/ (optional)
├── simulation/...
├── tests/...
├── tools/...
├── .github/workflows/
├── CMakeLists.txt
├── README.md
└── LICENSE
```

## Pin Assignments

### ESP32-S3 (Vision)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 10/11 | I2C0 (Thermal) | MLX90640 @ 0x33 |
| GPIO 12/13 | I2C1 (Reserved) | Future expansion |
| CSI_D0-D7 | Camera Parallel | OV5640 8-bit data |
| CSI_PCLK/VSYNC/HREF | Camera Control | OV5640 timing |
| GPIO 45/46 | UART0 (to C3) | 921600 baud, ESP-NOW backup |
| GPIO 3/4 | SPI (PSRAM) | If using external PSRAM |

### ESP32-C3 (Sensor Hub)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 6/7 | I2C0 (Sensors) | GPS, Mag, IMU, **BMP280 (0x76), AHT20 (0x38), Baro** |
| GPIO 8/9 | I2C1 (Reserved) | |
| GPIO 20/21 | UART0 (to S3) | 921600 baud |
| GPIO 18/19 | UART1 (to RP2040#1) | 115200 baud |
| GPIO 3/4 | UART2 (to RP2040#2) | 115200 baud |
| GPIO 5/6 | UART3 (to RP2040#3) | 115200 baud |
| GPIO 10 | GPIO (Status LED) | Heartbeat |

### RP2040 #1 (Motor Control + RC Input)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0/1 | UART0 (from C3) | Command interface |
| GPIO 2/3 | PWM Left Motor | TB6612FNG AIN1/AIN2 |
| GPIO 4/5 | PWM Right Motor | TB6612FNG BIN1/BIN2 |
| GPIO 6 | Enable Left | TB6612FNG PWMA |
| GPIO 7 | Enable Right | TB6612FNG PWMB |
| GPIO 8/9 | Encoder Left | Quadrature (if fitted) |
| GPIO 10/11 | Encoder Right | Quadrature (if fitted) |
| **GPIO 12** | **RC Ch1 (Throttle)** | **CP2 Receiver PWM In** |
| **GPIO 13** | **RC Ch2 (Steering)** | **CP2 Receiver PWM In** |
| **GPIO 14** | **RC Ch3 (Mode)** | **CP2 Receiver PWM In** |
| **GPIO 15** | **RC Ch4 (Aux)** | **CP2 Receiver PWM In** |

### RP2040 #2 (Steering + Front ToF)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0/1 | UART0 (from C3) | Command interface |
| GPIO 2 | Servo PWM | 50 Hz, 1-2ms pulse |
| GPIO 3/4 | I2C0 (ToF) | VL53L5CX @ 0x29 (front) |
| GPIO 5/6 | I2C1 (ToF) | VL53L5CX @ 0x29 (left) |
| GPIO 16 | GPIO (XSHUT) | VL53L5CX reset/selection |

### RP2040 #3 (Rear ToF + Aux)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0/1 | UART0 (from C3) | Command interface |
| GPIO 3/4 | I2C0 (ToF) | VL53L5CX @ 0x29 (rear) |
| GPIO 5/6 | I2C1 (ToF) | VL53L5CX @ 0x29 (right) |
| GPIO 16 | GPIO (XSHUT) | VL53L5CX reset/selection |

---

## Software Stack

| Layer | ESP32-S3 | ESP32-C3 | RP2040s |
|-------|----------|----------|---------|
| **Framework** | ESP-IDF / Arduino | ESP-IDF / Arduino | pico-sdk / Arduino |
| **RTOS** | FreeRTOS | FreeRTOS | FreeRTOS (pico-sdk) |
| **Comms** | ESP-NOW, UART | ESP-NOW, UART | UART |
| **Vision** | TFLite Micro, ESP-DL | — | — |
| **Sensor Fusion** | — | Custom (EKF) | — |
| **Motor Control** | — | — | PID + PIO PWM |

---

## Project Structure

```
drone/
├── docs/
│   ├── hardware-inventory.md
│   ├── wiring-diagrams/
│   ├── sensor-datasheets/
│   └── power-budget.md
├── firmware/
│   ├── esp32-s3-vision/
│   │   ├── CMakeLists.txt
│   │   ├── main/
│   │   │   ├── thermal_camera.c
│   │   │   ├── ir_camera.c
│   │   │   ├── vision_inference.c
│   │   │   └── comms_uart.c
│   │   └── components/
│   ├── esp32-c3-hub/
│   │   ├── CMakeLists.txt
│   │   ├── main/
│   │   │   ├── gps_gnss.c
│   │   │   ├── magnetometer.c
│   │   │   ├── imu_baro.c
│   │   │   ├── sensor_fusion_ekf.c
│   │   │   ├── comms_espnow.c
│   │   │   └── comms_uart_rp2040.c
│   │   └── components/
│   ├── rp2040-motor/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── motor_pid.c
│   │   │   ├── encoder.c
│   │   │   └── uart_cmd.c
│   │   └── pio/
│   ├── rp2040-steering-tof/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── servo.c
│   │   │   ├── vl53l5cx.c
│   │   │   └── uart_cmd.c
│   │   └── pio/
│   └── rp2040-rear-tof/
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── vl53l5cx.c
│       │   └── uart_cmd.c
│       └── pio/
├── simulation/
│   ├── gazebo/
│   └── ros2/
├── tests/
│   ├── unit/
│   └── integration/
├── tools/
│   ├── calibration/
│   ├── flashing/
│   └── monitoring/
├── .github/workflows/
├── CMakeLists.txt (top-level)
├── README.md
└── LICENSE
```

---

## Next Steps

1. **Finalize sensor selection** - Order thermal camera (MLX90640), GPS (u-blox), Mag (QMC5883L), ToF (VL53L5CX×4)
2. **Design PCB / perfboard layout** - Power distribution, level shifting, connector pinouts
3. **Implement ESP32-C3 sensor hub firmware** - GPS + Mag + IMU fusion (priority for navigation)
4. **Implement RP2040 motor control** - PID velocity control with encoder feedback
5. **Implement ESP32-S3 vision pipeline** - Thermal overlay + IR night vision + TFLite obstacle detection
6. **Integrate comms** - ESP-NOW S3↔C3, UART C3↔RP2040s
7. **Field test & calibrate** - Compass hard/soft iron, GPS RTK, thermal thresholds

---

## Power Budget (Estimated)

| Component | Current (mA) | Voltage | Power (mW) |
|-----------|-------------|---------|------------|
| ESP32-S3 (active, WiFi off) | 180 | 3.3V | 594 |
| ESP32-S3 (camera + PSRAM) | 350 | 3.3V | 1155 |
| ESP32-C3 (active, BLE) | 120 | 3.3V | 396 |
| RP2040 ×3 (active) | 3×25 | 3.3V | 248 |
| MLX90640 | 25 | 3.3V | 83 |
| OV5640 | 150 | 3.3V | 495 |
| u-blox ZED-F9P | 65 | 3.3V | 215 |
| QMC5883L | 5 | 3.3V | 17 |
| ICM-20948 | 3.5 | 3.3V | 12 |
| **BMP280** | **3.6** | **3.3V** | **12** |
| **AHT20** | **2.0** | **3.3V** | **7** |
| VL53L5CX ×4 | 4×20 | 3.3V | 264 |
| Motor Driver (idle) | 50 | 5V | 250 |
| Servo (moving) | 500 | 5V | 2500 |
| **Total (3.3V rail)** | ~1.21A | | **~4.02W** |
| **Total (5V rail, motors idle)** | ~0.55A | | **~2.75W** |
| **Peak (motors + servo)** | 10A+ | 7.4V | **~74W** |

> **Note:** Use separate 5V BEC (3A+) and 3.3V LDO (2A+) from main LiPo. Motor driver directly from LiPo.

---

## License

MIT License - See [LICENSE](LICENSE)