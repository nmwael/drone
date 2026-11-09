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
| **ESP8266MOD (AI Thinker)** | 1 | **Backup Comms / Telemetry Relay** | Xtensa L106, WiFi 4 (2.4 GHz only), 160KB SRAM, 4MB Flash |
| **RP2040** | 3 | **Available / Spare** | Dual-core Cortex-M0+, PIO, 264KB SRAM, 16MB/4MB Flash |

### Sensors (to procure/integrate)

| Sensor | Interface | Purpose | Candidate Parts |
|--------|-----------|---------|-----------------|
| **Thermal Camera** | I2C/SPI | Heat detection (8x8 to 32x32) | **REMOVED** — using OV5640 day camera only |
| **IR Camera (Night Vision)** | CSI/Parallel | Low-light imaging | OV2640/OV5640 with IR filter removed, ESP32-S3 CSI |
| **GNSS/GPS** | UART/I2C | Positioning | u-blox ZED-F9P (RTK), BN-880, SAM-M10Q |
| **Magnetometer (Compass)** | I2C | Heading | QMC5883L, HMC5883L, RM3100, LIS3MDL |
| **IMU (Accel/Gyro)** | I2C/SPI | Orientation, vibration | ICM-20948, BMI270, MPU6050 |
| **Barometer / Pressure** | I2C | Altitude, pressure | **BMP280** (on hand), BMP390, MS5611 |
| **Temperature / Humidity** | I2C | Ambient temp & RH | **AHT20** (on hand), SHT40, BME280 |

### Power & Actuation

| Component | Specs | Notes |
|-----------|-------|-------|
| **Main Battery** | 2S/3S LiPo (7.4V/11.1V) | Power distribution box needed (printed) |
| **BEC / 5V Reg** | 5V 3A+ | For MCUs and sensors |
| **3.3V Reg** | 3.3V 1A+ | For ESP32-C3/S3, sensors |
| **Nokia BP-4L Cells** | Multiple × 3.7V ~1500mAh | **ON HAND** — Backup/aux power for electronics, gimbal servos, testing |
| **libr160 Li-ion Cells** | 2 × 3.7V 1050mAh | **ON HAND** — Electronics backup, sensor power, ESP32 testing |

> **Note:** Truck drivetrain uses **existing brushed ESC** (driven directly by CP2 receiver). Steering uses **existing servo** (driven directly by CP2 receiver). No additional motor driver or servo controller needed.

### Salvaged Parts: Honeybee CP2 (Coaxial Helicopter)

| Part | Qty | Condition | Reuse Potential |
|------|-----|-----------|-----------------|
| **2.4GHz Receiver** | 1 | Working | Direct PWM/SBUS output to ESC + steering servo |
| **Main Motor (Brushed)** | 1 | Working | Low-voltage test loads, winch |
| **Tail Motor (Brushed)** | 1 | Working | Auxiliary actuator |
| **Main Rotor Servos** | 2 | Working | Camera gimbal pan/tilt (small, ~1.5kg-cm) |
| **Gyro (Piezo/MEMS)** | 1 | Working | Rate damping reference (if analog/PWM) |
| **ESC (Brushed, built-in)** | 1 | Working | Motor driver reference / low-power loads |
| **LiPo 1S/2S Pack** | 1 | Unknown | Test power source |
| **Charger** | 1 | Working | 1S/2S LiPo charging |
| **Propellers** | 0 | **Broken** | Replace if flying again |

> **Note:** The CP2 receiver outputs standard PWM (1-2ms) or SBUS. It connects **directly to the truck's ESC and steering servo** — no MCU in the control loop. MCUs are purely for sensing and streaming.

### ESP8266MOD (AI Thinker) — Secondary Roles

| Role | Description | Interface |
|------|-------------|-----------|
| **Telemetry Relay** | Independent low-rate telemetry uplink (MAVLink/JSON) via WiFi to ground station | UART from C3 @ 115200 -> ESP8266 -> UDP/TCP |
| **Watchdog / Heartbeat** | Monitors ESP32-C3/S3 health; triggers hardware reset via GPIO if stalled | GPIO -> C3/S3 EN/RST pins |
| **OTA Bootstrap** | Serves firmware images for ESP32-C3/S3 OTA updates over local WiFi | SPIFFS/LittleFS + HTTP server |
| **WiFi Range Extender** | ESP-NOW -> WiFi bridge for longer-range telemetry (2.4 GHz only) | ESP-NOW from C3 -> ESP8266 -> WiFi AP |
| **Config / Debug Portal** | Captive portal for WiFi credentials, sensor calibration, log download | Web server on ESP8266 |

> **Limitation:** ESP8266 is **single-core, no BLE, no hardware crypto, 2.4 GHz only, limited RAM**. Not suitable for video, STANAG encoding, or real-time control. Use as **auxiliary/backup only**.

### Existing RC Monster Truck ON HAND — **100% DRIVING FUNCTIONAL**

| Aspect | Details |
|--------|---------|
| **Chassis** | Using as-is — no modifications needed |
| **Remote** | Honeybee CP2 2.4GHz transmitter + receiver (working perfectly) |
| **Drivetrain** | Existing brushed motor + ESC — **CP2 receiver → ESC (direct, works)** |
| **Steering** | Existing servo — **CP2 receiver → steering servo (direct, works)** |
| **Body** | Existing shell — design printed mounts to use existing body clips/mounting points |
| **Battery** | Uses existing battery compartment (2S/3S LiPo upgrade optional) |

> **IMPORTANT:** The truck drives **perfectly right now** with the CP2 remote. Zero modifications to drivetrain, steering, ESC, or control system. This project **only adds sensors + compute** (vision, thermal, GPS, IMU, ToF) for autonomous/teleop capability. The CP2 remote remains the primary/fallback control — always functional regardless of MCU state.

### Universal Electronics Box — Single Modular Enclosure (FDM Printer ON HAND)

**Concept:** One weatherproof box containing ALL electronics — mount on ANY vehicle via universal bolt pattern.

| Part | Qty | Material | Purpose | Mounting Strategy |
|------|-----|----------|---------|-------------------|
| **Universal Electronics Box** | 1 | PETG + TPU gasket | **All-in-one:** ESP32-S3, ESP32-C3, ESP8266, power distro (2× buck converters), BMS for spare cells, sensor hub (GPS/IMU/Mag/Baro/AHT20), all internal wiring | **Universal mount:** 4× M6 slots (50-100mm spacing) + 4× M8 slots (100-150mm) — fits most vehicle frames, roof racks, roll cages. 4× M5 camera gimbal mount on front face. |
| **Camera Gimbal (2-axis)** | 1 | PETG + TPU | Pan/tilt for OV5640 day camera | Mounts to Universal Box front face (4× M5) |
| **OV5640 Mount** | 1 | PETG | Align day camera | Integrated into gimbal |
| **Battery Tray / Strap** | 1 | TPU | Secure 2S/3S LiPo (main vehicle battery) | Separate — uses existing battery compartment |
| **GPS Antenna Mast** | 1 | PETG/ASA | Elevate ZED-F9P antenna | Mounts to Universal Box top rear |
| **ESP32-S3 Antenna Mount** | 1 | PETG | External u.FL/SMA antenna | Mounts to Universal Box side |
| **Vibration Dampers** | 4 | TPU (60-80A) | Isolate Universal Box from vehicle | 4× corner mounts between box and vehicle frame |
| **Wire Routing (Internal)** | — | PETG | Cable management inside box | Printed channels + clips |
| **Servo Horn Adapters** | 2 | PETG | CP2 servo -> gimbal (pan/tilt) | Match existing servo spline (measure!) |

> **Universal Box Internal Layout:**
> - **Top:** GPS antenna mast, ESP32-S3 antenna, ventilation slots (TPU mesh)
> - **Front:** Camera gimbal mount (4× M5), OV5640 day camera
> - **Inside Left:** ESP32-S3 (vision), ESP8266 (backup comms)
> - **Inside Center:** ESP32-C3 (sensor hub), sensor hub PCB (GPS/IMU/Mag/Baro/AHT20)
> - **Inside Right:** Power distribution — 2× buck converters (3.3V/5V), BMS for spare cells, XT60 input, fused outputs
> - **Bottom:** 4× M6/M8 mounting slots (universal), 4× TPU damper mounts
> - **Sealing:** TPU gasket groove, IP66 target
> - **Connectors (external):** XT60 power in, SMA GPS, SMA WiFi, M12 sensor bus (optional)

> **Design Workflow:** 1) Model in Fusion 360/FreeCAD. 2) Print test fits in PLA. 3) Final prints: Box in PETG, gaskets/dampers in TPU. 4) Install heat-set inserts (M3/M4/M5/M6) for bolts. 5) Assemble, wire, test. 6) Mount on any vehicle in <10 min.

### Manufacturing Capability: FDM 3D Printer ON HAND

| Capability | Use Cases for This Project |
|------------|---------------------------|
| **Print Volume** | Assumed >= 220x220x250mm (Ender 3 class or larger) |
| **Materials ON HAND** | ✅ **PLA** (prototypes), ✅ **PETG** (functional), ✅ **TPU** (vibration dampening) |
| **Materials OPTIONAL** | ASA/ABS (heat resistant, outdoor/UV) |
| **Resolution** | 0.1-0.3mm layer height, 0.4mm nozzle standard |

#### Printable Parts (replaces purchased items)

| Part | Qty | Material | Est. Print Time | Replaces |
|------|-----|----------|-----------------|----------|
| **Universal Electronics Box** | 1 | PETG + TPU | 12-18h | All separate enclosures (electronics tray, C3 hub, power distro) |
| **Camera Gimbal (2-axis)** | 1 | PETG + TPU | 6-8h | Pan/tilt for vision sensors (uses CP2 servos) |
| **OV5640 Mount** | 1 | PETG | 2-3h | Camera alignment bracket |
| **GPS Antenna Mast** | 1 | PETG/ASA | 2h | Elevate ZED-F9P antenna |
| **ESP32-S3 Antenna Mount** | 1 | PETG | 1h | External u.FL/SMA antenna |
| **Vibration Dampers** | 4 | TPU (60-80A) | 30min each | Universal Box isolation |
| **Wire Routing (Internal)** | — | PETG | 1-2h | Cable management |
| **Servo Horn Adapters** | 2 | PETG | 30min each | CP2 servo -> gimbal |

> **Design Tip:** Use **heat-set inserts (M3/M4/M5/M6)** for bolt-on strength. Print box with **4-5 walls, 30-40% gyroid infill, 0.2mm layers**. Print gaskets/dampers in TPU (Shore 60-80A). Anneal PETG at 80°C for 30min if heat resistance needed.

## System Architecture

```
+-----------------------------------------------------------------+
|                      ESP32-S3 (Vision)                          |
|  +-----------------+  +-----------------+  +-----------------+  |
|  | Thermal Cam     |  |  IR Camera      |  | Vision Process  |  |
|  |  (MLX90640)     |  |  (OV5640)       |  |  (TensorFlow)   |  |
|  +--------+--------+  +--------+--------+  +--------+--------+  |
+----------|----------------|----------------|----------------|----+
           |                |                |                |
           v                v                v                v
+-----------------------------------------------------------------+
|                    ESP32-C3 (Sensor Hub)                        |
|  +-----------------+  +-----------------+  +-----------------+  |
|  |   GPS/GNSS      |  | Magnetometer    |  |    IMU + Baro   |  |
|  |  (u-blox)       |  |  (QMC5883)      |  |   (ICM-20948)   |  |
|  +--------+--------+  +--------+--------+  +--------+--------+  |
|           |                |                |                |
|           v                v                v                v
|  +-----------------------------------------------------------------+  |
|  |              Sensor Fusion EKF                                  |  |
|  |         + AHT20 (Temp/RH) + BMP280 (Press)                     |  |
|  +-----------------------------------------------------------------+  |
+--------------------------|--------------------------------------------+
                           |
           +---------------+---------------+
           v               v               v
+------------------+ +------------------+ +------------------+
|  RP2040 #1       | |  RP2040 #2       | |  RP2040 #3       |
|  (16MB Flash)    | |  (4MB Flash)     | |  (4MB Flash)     |
|  +-------------+ | |  +-------------+ | |  +-------------+ |
|  | 4x VL53L5CX | | |  | 4x VL53L5CX | | |  | 4x VL53L5CX | |
|  | ToF Array   | | |  | ToF Array   | | |  | ToF Array   | |
|  | Front/      | | |  | Left/       | | |  | Rear/       | |
|  | Rear/       | | |  | Right/      | | |  | Aux         | |
|  | Sides       | | |  | Aux         | | |  |             | |
|  +-------------+ | |  +-------------+ | |  +-------------+ |
+--------|---------+ +--------|---------+ +--------|---------+
         |                       |                       |
         +-----------------------+-----------------------+
                                 v
                +----------------------------------+
                |     ESP32-C3 (Aggregation)       |
                |  Sensor Fusion + ToF Data        |
                |  -> ESP-NOW -> ESP32-S3          |
                |  -> UART -> ESP8266 (Backup)     |
                +----------------------------------+
                                 ^
                                 | UART (telemetry, watchdog)
                                 |
                    +------------+------------+
                    |  ESP8266MOD             |
                    |  (AI Thinker)           |
                    |  - Telemetry UDP        |
                    |  - Watchdog RST         |
                    |  - OTA Server           |
                    +-------------------------+
```

**Note:** Truck drivetrain & steering are driven **directly by CP2 receiver** -> existing ESC & servo. No MCU in the control loop.

### Communication Bus

| Link | Protocol | Rate | Purpose |
|------|----------|------|---------|
| ESP32-S3 <-> ESP32-C3 | ESP-NOW / UART | 1-2 Mbps | Vision <-> Sensor fusion, telemetry |
| ESP32-C3 -> ESP8266MOD | UART | 115200 baud | Backup telemetry, watchdog, OTA |
| ESP8266MOD -> Ground | WiFi UDP/TCP | ~1 Mbps | Backup telemetry, config portal |
| CP2 Receiver -> ESC/Servo | PWM / SBUS | 50 Hz / 100 kbps | **Direct drive** (no MCU in loop) |

### Control Modes

| Mode | Source | Path | Use Case |
|------|--------|------|----------|
| **Autonomous** | ESP32-S3/C3 | Sensor fusion -> Waypoint planner | Full autonomy (vision-based) |
| **Teleop (WiFi)** | Phone/PC / Quest | WiFi -> ESP32-S3 -> ESP-NOW -> C3 -> Sensor streams | FPV + sensor overlay |
| **RC Manual** | CP2 Transmitter | **PWM/SBUS -> ESC + Steering Servo** | **Direct, no MCU latency** |

> **Safety:** CP2 receiver has **direct electrical connection** to ESC and steering servo. MCU failure = full manual control retained. Autonomous/Teleop modes send **advisory commands** (speed limit, heading hold) via ESP32-C3 GPIO to CP2 receiver's trainer port if available, else purely sensor streaming.

---

## STANAG 4609 Video Encoding & Oculus Rift Teleop

### STANAG 4609 / MISB Compliance

| Requirement | Specification | Implementation |
|-------------|---------------|----------------|
| **Video Codec** | H.264 (AVC) High Profile @ Level 4.1 / MPEG-2 | ESP32-S3 hardware H.264 encoder (max 1080p@30fps) |
| **Container** | MPEG-TS (Transport Stream) | libavformat / custom TS muxer on S3 |
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

> **Note:** ESP32-S3 has **hardware H.264 encoder** (max 1920x1080 @ 30fps). For STANAG compliance, use 1280x720 @ 30fps @ 4-6 Mbps CBR. KLV injection requires TS packet manipulation — do this in ESP-IDF on S3 before WiFi transmit.

---

### Oculus Rift (Meta Quest) Teleop Architecture

```
+-----------------------------------------------------------------+
|                           OCULUS QUEST 2/3/Pro                  |
|  +-----------------------------------------------------------+  |
|  |              WebXR / OpenXR Runtime (Browser/App)         |  |
|  |  +-------------+  +-------------+  +-------------+        |  |
|  |  | Left Eye    |  | Right Eye   |  | Head Tracking|        |  |
|  |  | (1280x1280) |  | (1280x1280) |  | (6DoF, 90Hz) |        |  |
|  |  +------+------+  +------+------+  +------+------+        |  |
|  +---------|----------------|----------------|---------------+  |
+-----------|----------------|----------------|------------------+
            |                |                |
            v                v                v
+-----------------------------------------------------------------+
|                      GROUND STATION (PC / Raspberry Pi 5)       |
|  +---------------------+  +---------------------+  +----------+ |
|  | WebRTC / WHIP Server|  | STANAG TS Demux +   |  | WebXR    | |
|  | (MediaMTX /         |  | KLV Parser          |  | Teleop   | |
|  |  GStreamer)         |  |  - Extract KLV      |  | App      | |
|  |  - Ingest: UDP/TCP  |  |  - Overlay UI       |  | (Three.js| |
|  |  - Transcode: H.264 |  |  - Record .ts/.klv  |  |  + WebXR)| |
|  |  - Relay: WebRTC    |  |                     |  |          | |
|  +----------+----------+  +----------+----------+  +----+-----+ |
+-------------|-------------------------|--------------------|------+
              |                         |                    |
              v                         v                    v
+-----------------------------------------------------------------+
|                         NETWORK (WiFi 5/6 / 4G/5G)              |
|  +---------------------+  +---------------------+  +----------+ |
|  | Video Stream        |  | Telemetry Downlink  |  | Control  | |
|  | H.264 TS @ 4-6 Mbps |  | KLV + MAVLink @     |  | Uplink   | |
|  | UDP / RTP           |  | 10-50 Hz            |  | WebRTC   | |
|  +---------------------+  +---------------------+  | DataChan | |
|                                                     +----------+ |
+-----------------------------------------------------------------+
              |                         |                    |
              v                         v                    v
+-----------------------------------------------------------------+
|                            ESP32-S3 (Onboard)                   |
|  +---------------------+  +---------------------+  +----------+ |
|  | Camera Capture      |  | H.264 Encoder       |  | WiFi TX  | |
|  | OV5640 @ 720p30     |  | HW Enc @ 4-6 Mbps   |  | ESP-NOW  | |
|  | Thermal Overlay     |  | KLV Injection       |  | / WiFi   | |
|  |                     |  |                     |  | UDP      | |
|  +---------------------+  +---------------------+  +----------+ |
+-----------------------------------------------------------------+
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
| WebRTC decode + jitter | 30-50 ms | 20 ms | MediaMTX + webrtc-unidirectional |
| Browser decode + render | 15-20 ms | 15 ms | WebCodecs API preferred |
| **Total** | **123-188 ms** | **~111 ms** | With optimizations |

> **Critical:** Use **ESP-NOW** (not WiFi TCP) for video uplink. Ground station runs MediaMTX (Go) or GStreamer to ingest UDP -> WebRTC -> Browser WebXR.

---

### Video Streaming Pipeline (ESP32-S3 -> Oculus)

```
ESP32-S3 Firmware (ESP-IDF):
+-----------------------------------------------------------------+
|  OV5640 (CSI/Parallel)                                          |
|       |                                                         |
|       v                                                         |
|  ISP / DMA -> YUV420 buffers (double-buffered)                  |
|       |                                                         |
|       v                                                         |
|  H.264 Hardware Encoder (ESP32-S3 ENC)                          |
|       |  - Profile: High @ Level 4.1                            |
|       |  - Resolution: 1280x720 @ 30fps                         |
|       |  - Bitrate: 4 Mbps CBR                                  |
|       |  - GOP: 30 (I-frame every 1s)                           |
|       v                                                         |
|  KLV Builder (MISB ST 0601)                                     |
|       |  - GPS/IMU from C3 via UART (10 Hz)                     |
|       |  - Timestamp from GPS PPS + RTC                         |
|       v                                                         |
|  MPEG-TS Muxer (188-byte packets, PID 0x100 video, 0x101 KLV)   |
|       |                                                         |
|       v                                                         |
|  ESP-NOW / UDP Socket -> Ground Station IP:Port                 |
+-----------------------------------------------------------------+
```

#### Ground Station: MediaMTX Config (mediamtx.yml)

```yaml
paths:
  stanag:
    source: udp://0.0.0.0:8888
    sourceFormat: mpegts
    sourceOnDemand: false
    webrtc: true
    webrtcTransport: udp
    webrtcICEServers:
      - urls: stun:stun.l.google.com:19302
    record: true
    recordPath: ./recordings/%path/%Y-%m-%d_%H-%M-%S.ts
    recordFormat: mpegts
    runOnInit: ./scripts/klv_extractor.sh %path
```

---

### Oculus WebXR Teleop App

| Component | Technology | Repo Path |
|-----------|------------|-----------|
| **Rendering** | Three.js (WebGL2) + WebXR Layers | `ground-station/webxr-app/` |
| **Video Decode** | WebCodecs API (hardware H.264 decode) | `ground-station/webxr-app/src/video/` |
| **Stereo** | Side-by-side / Top-bottom from single stream | Shader-based split |
| **Head Tracking** | `xrFrame.getViewerPose()` -> yaw/pitch/roll | Maps to camera gimbal (future) |
| **Controller Input** | `xrFrame.getPose(controller.gripSpace)` | Throttle/steering/mode |
| **Data Channel** | WebRTC DataChannel (reliable, ordered) | Commands @ 50 Hz |
| **Telemetry Overlay** | KLV parse -> HUD (lat/lon/alt/heading/thermal) | Three.js CSS2DRenderer |

#### Control Mapping (Quest 2/3 Controllers)

| Input | Axis/Button | Truck Command |
|-------|-------------|---------------|
| **Left Thumbstick Y** | Axis [-1, 1] | Throttle (forward/back) |
| **Right Thumbstick X** | Axis [-1, 1] | Steering (left/right) |
| **Right Trigger** | Axis [0, 1] | Speed modifier (boost) |
| **Left Trigger** | Axis [0, 1] | Brake / reverse |
| **A Button** | Press | Mode: Autonomous |
| **B Button** | Press | Mode: Teleop |
| **X Button** | Press | Mode: RC Manual |
| **Y Button** | Press | Thermal overlay toggle |
| **Left Grip** | Squeeze | Gimbal pitch down (future) |
| **Right Grip** | Squeeze | Gimbal pitch up (future) |
| **Head Yaw** | 6DoF pose | Camera pan (future gimbal) |
| **Head Pitch** | 6DoF pose | Camera tilt (future gimbal) |

> **Safety:** WebXR app sends `HEARTBEAT` @ 10 Hz. Loss > 500 ms -> truck enters **RC Manual** (direct CP2 control).

---

### Hardware Additions for STANAG + Oculus

| Item | Qty | Purpose | Notes |
|------|-----|---------|-------|
| **ESP32-S3 with PSRAM** | 1 | **Required** | H.264 encoder needs PSRAM for frame buffers (min 2MB, 8MB preferred) |
| **OV5640 Camera Module** | 1 | 720p/1080p @ 30fps | CSI/parallel; IR filter removed for day camera |
| **u-blox ZED-F9P + GPS Antenna** | 1 | Precision timestamp + position | PPS output to ESP32-S3 GPIO for frame sync |
| **WiFi 5 Router / AP** | 1 | Ground link | **ON HAND** — existing router (5 GHz capable) |
| **Ground Station PC / Pi 5** | 1 | MediaMTX + WebXR host | Pi 5 can run MediaMTX; PC needed for WebXR browser |
| **Meta Quest 2** | 1 | HMD + Controllers | **ON HAND** — used Quest 2 ($300 value), WebXR capable |
| **External Antenna (ESP32-S3)** | 1 | Range | 2.4/5 GHz dual-band PCB antenna or u.FL -> SMA |

---

### Updated Power Budget (with Video + STANAG)

| Component | Current (mA) | Voltage | Power (mW) |
|-----------|-------------|---------|------------|
| ESP32-S3 (WiFi + H.264 enc + PSRAM) | 550 | 3.3V | 1815 |
| OV5640 Camera | 180 | 3.3V | 594 |
| ESP32-C3 (Sensor Hub) | 120 | 3.3V | 396 |
| RP2040 x3 | 75 | 3.3V | 248 |
| GPS (ZED-F9P) | 65 | 3.3V | 215 |
| IMU + Mag + Baro + 12x ToF | 180 | 3.3V | 594 |
| **BMP280** | **3.6** | **3.3V** | **12** |
| **AHT20** | **2.0** | **3.3V** | **7** |
| **Total (3.3V rail)** | **~1.18A** | | **~3.88W** |

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
│   │   │   ├── aht20.c                 # NEW: Temp/Humidity sensor
│   │   │   ├── bmp280.c                # NEW: Pressure/Temp sensor
│   │   │   ├── sensor_fusion_ekf.c
│   │   │   ├── comms_espnow.c
│   │   │   ├── comms_uart_rp2040.c
│   │   │   └── telemetry_provider.c    # NEW: KLV data source for S3
│   │   └── components/
│   ├── rp2040-tof-1/                   # RP2040 #1 - 16MB Flash
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── vl53l5cx_4x.c
│   │   │   ├── uart_cmd.c
│   │   │   └── i2c_mux.c
│   │   └── pio/
│   ├── rp2040-tof-2/                   # RP2040 #2 - 4MB Flash
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── vl53l5cx_4x.c
│   │   │   ├── uart_cmd.c
│   │   │   └── i2c_mux.c
│   │   └── pio/
│   └── rp2040-tof-3/                   # RP2040 #3 - 4MB Flash
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── vl53l5cx_4x.c
│       │   │   ├── uart_cmd.c
│       │   └── i2c_mux.c
│       └── pio/
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

1. **Finalize sensor selection** - Order thermal camera (MLX90640), GPS (u-blox), Mag (QMC5883L), ToF (12x VL53L5CX)
2. **Design PCB / perfboard layout** - Power distribution, level shifting, connector pinouts
3. **Implement ESP32-C3 sensor hub firmware** - GPS + Mag + IMU + AHT20 + BMP280 fusion (priority for navigation)
4. **Implement RP2040 ToF firmware** - 4x VL53L5CX per RP2040, I2C mux, UART streaming
5. **Implement ESP32-S3 vision pipeline** — Day camera + H.264 + KLV + TS
6. **Integrate comms** - ESP-NOW S3<->C3, UART C3<->RP2040s, UART C3->ESP8266
7. **Field test & calibrate** - Compass hard/soft iron, GPS RTK, thermal thresholds, ToF zones

---

## Power Budget (Estimated)

| Component | Current (mA) | Voltage | Power (mW) |
|-----------|-------------|---------|------------|
| ESP32-S3 (active, WiFi off) | 180 | 3.3V | 594 |
| ESP32-S3 (camera + PSRAM) | 350 | 3.3V | 1155 |
| ESP32-C3 (active, BLE) | 120 | 3.3V | 396 |
| RP2040 x3 (active) | 3x25 | 3.3V | 248 |
| OV5640 | 150 | 3.3V | 495 |
| u-blox ZED-F9P | 65 | 3.3V | 215 |
| QMC5883L | 5 | 3.3V | 17 |
| ICM-20948 | 3.5 | 3.3V | 12 |
| **BMP280** | **3.6** | **3.3V** | **12** |
| **AHT20** | **2.0** | **3.3V** | **7** |
| VL53L5CX x12 | 12x20 | 3.3V | 792 |
| **Total (3.3V rail)** | **~1.21A** | | **~3.99W** |

> **Note:** Use separate 5V BEC (3A+) and 3.3V LDO (2A+) from main LiPo. Existing truck ESC runs directly from LiPo.

---

## License

MIT License - See [LICENSE](LICENSE)