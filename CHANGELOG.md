# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Root-level AI onboarding: `AGENTS.md`, `QUICK-REFERENCE.md`
- STANAG S3 Simulator AI onboarding: `firmware/stanag-s3-simulator/AGENTS.md`, `QUICK-REFERENCE.md`
- STANAG S3 Simulator glossary: `firmware/stanag-s3-simulator/GLOSSARY.md`
- STANAG S3 Simulator troubleshooting: `firmware/stanag-s3-simulator/TROUBLESHOOTING.md`
- Contribution guide: `CONTRIBUTING.md`
- Updated root `README.md` with subsystem index and active firmware status

### Changed
- Root `README.md` now properly indexes STANAG S3 Simulator as active firmware
- Improved cross-referencing between root and component documentation

## [0.1.0] - 2024-11-16

### Added - STANAG S3 Simulator (Initial Release)
- ESP32-S3 firmware for STANAG 4609 compliance testing
- 5 JPEG frame loop from SPIFFS (1280×720)
- Simulated sensors: GPS (RTK), IMU (9-DoF), Magnetometer, Barometer, AHT20
- Software H.264 encoder (High Profile L4.1) with valid SPS/PPS/IDR/AUD NAL units
- MISB ST 0601 KLV metadata builder (UAS Local Set + custom tags)
- MPEG-TS muxer with PAT/PMT/PES (PID 0x0000, 0x1000, 0x0100, 0x0101, 0x1FFF)
- ESP-NOW / UDP streaming to ground station
- UART telemetry (JSON @ 115200 baud) to ESP32-C3
- PSRAM support for YUV frame buffers
- 64 Unity unit tests across 7 modules
- SPIFFS image generation from `data/` JPEG frames
- Custom partition table with ~960KB SPIFFS partition
- MediaMTX ground station configuration for WebRTC + Meta Quest
- KLV extraction hook for MediaMTX
- TS/KLV parser for serial log analysis (`scripts/parse_ts_output.py`)

### Documentation
- `README.md` — Quick start, features, hardware
- `OVERVIEW.md` — Architecture with Mermaid diagrams
- `DATA-FLOW.md` — Per-frame pipeline with timing budget
- `BUILD.md` — Build guide and troubleshooting
- `TESTING.md` — Unit test guide (64 tests)
- `API.md` — Complete API reference
- `MEDIAMTX.md` — Ground station setup (MediaMTX + WebRTC + Quest)
- `mediamtx.yml` — Ready-to-use MediaMTX config

### Hardware Tested
- ESP32-S3 DevKit (PSRAM required, 8MB+)
- USB serial at 115200 baud
- Output verified: JSON telemetry + MPEG-TS hex dumps

---

## Versioning Scheme

- **Major** — Breaking API changes, architecture overhaul
- **Minor** — New features, new subsystems, significant enhancements
- **Patch** — Bug fixes, documentation updates, minor improvements

## Release Process

1. Update `CHANGELOG.md` with new version
2. Tag release: `git tag v<major>.<minor>.<patch>`
3. Push tag: `git push origin v<major>.<minor>.<patch>`
4. CI builds artifacts and creates release

---

## Subsystem Versions

| Subsystem | Version | Status |
|-----------|---------|--------|
| STANAG S3 Simulator | 0.1.0 | ✅ Running |
| ESP32-S3 Vision | — | 🚧 Planned |
| ESP32-C3 Sensor Hub | — | 🚧 Planned |
| RP2040 ToF ×3 | — | 🚧 Planned |
| MediaMTX Config | 0.1.0 | ✅ Ready |
| WebXR App | — | 🚧 Scaffold |