# STANAG/MISB Glossary — STANAG S3 Simulator

## Standards

| Term | Definition |
|------|------------|
| **STANAG 4609** | NATO Standardization Agreement 4609 — "Airborne Intelligence, Surveillance, and Reconnaissance (ISR) Video Format" |
| **MISB ST 0601** | Motion Imagery Standards Board Standard 0601 — "UAS Local Set" metadata definition for STANAG 4609 |
| **KLV** | Key-Length-Value — binary encoding: 2-byte Key (big-endian), 2-byte Length (big-endian), Value bytes |
| **MPEG-TS** | MPEG Transport Stream — 188-byte packets with PID routing, used for STANAG 4609 container |
| **PAT** | Program Association Table — PID 0x0000, maps program numbers to PMT PIDs |
| **PMT** | Program Map Table — PID 0x1000 (typical), maps stream types to elementary PIDs |
| **PES** | Packetized Elementary Stream — variable-length payload carrying encoded data (video/KLV) |
| **PCR** | Program Clock Reference — timing reference in TS adaptation field (27 MHz clock) |
| **PID** | Packet Identifier — 13-bit field in TS header routing packets to streams |

## Video Encoding

| Term | Definition |
|------|------------|
| **H.264 / AVC** | Advanced Video Coding — ITU-T H.264 / ISO/IEC 14496-10 |
| **High Profile** | H.264 profile with 8×8 transform, CABAC, interlaced support |
| **Level 4.1** | Max 1280×720@60fps or 1920×1080@30fps, 50 Mbps max bitrate |
| **NAL Unit** | Network Abstraction Layer unit — H.264 packet with type header |
| **SPS** | Sequence Parameter Set — NAL type 7, codec configuration |
| **PPS** | Picture Parameter Set — NAL type 8, picture-level config |
| **IDR** | Instantaneous Decoding Refresh — NAL type 5, I-frame (keyframe) |
| **AUD** | Access Unit Delimiter — NAL type 9, frame boundary marker |
| **GOP** | Group of Pictures — distance between IDR frames |

## Sensor Terms

| Term | Definition |
|------|------------|
| **GPS / GNSS** | Global Navigation Satellite System — position/velocity/time |
| **RTK** | Real-Time Kinematic — cm-level GPS accuracy using base station corrections |
| **Fix Type** | 0=none, 1=GPS, 2=DGPS, 3=3D, 4=RTK Fixed, 5=RTK Float |
| **TOW** | Time of Week — GPS time (ms since Sunday 00:00) |
| **WGS-84** | World Geodetic System 1984 — standard Earth ellipsoid for GPS |
| **deg7** | Degrees × 10⁷ — MISB encoding for lat/lon (int32) |
| **deg5** | Degrees × 10⁵ — MISB encoding for heading (int32) |
| **IMU** | Inertial Measurement Unit — accelerometer + gyroscope |
| **9-DoF** | 9 Degrees of Freedom — 3-axis accel + 3-axis gyro + 3-axis mag |
| **EKF** | Extended Kalman Filter — sensor fusion algorithm |
| **Quaternion** | 4-element rotation representation (w, x, y, z) avoiding gimbal lock |
| **NED** | North-East-Down — local tangent plane coordinate frame |
| **Magnetometer** | Measures Earth's magnetic field for heading |
| **Hard/Soft Iron** | Magnetic distortions requiring calibration |
| **Barometer** | Measures atmospheric pressure for altitude |
| **Lapse Rate** | Temperature decrease with altitude (~6.5°C/km) |

## ESP32-S3 Specific

| Term | Definition |
|------|------------|
| **PSRAM** | Pseudo-Static RAM — external SPI RAM (8 MB on ESP32-S3) |
| **SPIFFS** | SPI Flash File System — embedded filesystem on flash |
| **ESP-NOW** | Espressif proprietary low-latency peer-to-peer protocol |
| **ESP-IDF** | Espressif IoT Development Framework — official SDK |
| **EIM** | ESP-IDF Manager — version manager for ESP-IDF |
| **QEMU** | Quick EMUlator — ESP32 emulation for CI/testing |
| **Unity** | C unit test framework used by ESP-IDF |
| **MALLOC_CAP_SPIRAM** | Heap capability flag for PSRAM allocations |

## Network/Streaming

| Term | Definition |
|------|------------|
| **MediaMTX** | Lightweight Go media server (RTSP/WebRTC/SRT/HLS) |
| **WebRTC** | Web Real-Time Communication — browser-based ultra-low-latency streaming |
| **STUN** | Session Traversal Utilities for NAT — NAT traversal helper |
| **TURN** | Traversal Using Relays around NAT — relay for difficult NAT |
| **ICE** | Interactive Connectivity Establishment — NAT traversal framework |
| **SDP** | Session Description Protocol — media capability negotiation |
| **WHIP** | WebRTC-HTTP Ingestion Protocol — standardized WebRTC ingest |

## Meta Quest / WebXR

| Term | Definition |
|------|------------|
| **WebXR** | Web API for VR/AR (immersive-web.github.io/webxr) |
| **WebCodecs** | Browser API for hardware video decode (VideoDecoder) |
| **6DoF** | Six Degrees of Freedom — position (x,y,z) + rotation (yaw,pitch,roll) |
| **XR Frame** | Single rendered frame for one eye in WebXR session |
| **DataChannel** | WebRTC reliable/unordered channel for control/telemetry |

## Units & Encodings

| Term | Definition |
|------|------------|
| **Big-endian** | Most significant byte first (network byte order) |
| **Little-endian** | Least significant byte first (x86, ARM, ESP32 native) |
| **mm** | Millimeters — MISB altitude/height unit |
| **mm/s** | Millimeters per second — MISB velocity unit |
| **deg7** | Degrees × 10,000,000 — MISB lat/lon |
| **deg5** | Degrees × 100,000 — MISB heading |
| **µs** | Microseconds — timestamp resolution |
| **CBR** | Constant Bit Rate |
| **VBR** | Variable Bit Rate |