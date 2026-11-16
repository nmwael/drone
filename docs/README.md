# Documentation Index

## 🎯 Active & Running
- **[STANAG S3 Simulator](../firmware/stanag-s3-simulator/README.md)** — ESP32-S3 STANAG 4609 POC **currently on `/dev/ttyACM0`**
  - [Architecture Overview](../firmware/stanag-s3-simulator/OVERVIEW.md)
  - [Data Flow & Timing](../firmware/stanag-s3-simulator/DATA-FLOW.md)
  - [API Reference](../firmware/stanag-s3-simulator/API.md)
  - [Build Guide](../firmware/stanag-s3-simulator/BUILD.md)
  - [Testing Guide](../firmware/stanag-s3-simulator/TESTING.md)
  - [MediaMTX Ground Station](../firmware/stanag-s3-simulator/MEDIAMTX.md)
  - [Glossary](../firmware/stanag-s3-simulator/GLOSSARY.md)
  - [Troubleshooting](../firmware/stanag-s3-simulator/TROUBLESHOOTING.md)
  - [AI Onboarding](../firmware/stanag-s3-simulator/AGENTS.md)
  - [Quick Reference](../firmware/stanag-s3-simulator/QUICK-REFERENCE.md)

## Firmware (Planned — Now with AI Onboarding)
- **[ESP32-S3 Vision](../firmware/esp32-s3-vision/README.md)** — STANAG H.264 + Thermal/IR + TFLite
  - [AI Onboarding](../firmware/esp32-s3-vision/AGENTS.md)
  - [Quick Reference](../firmware/esp32-s3-vision/QUICK-REFERENCE.md)
- **[ESP32-C3 Sensor Hub](../firmware/esp32-c3-hub/README.md)** — GPS + IMU + Mag + EKF
  - [AI Onboarding](../firmware/esp32-c3-hub/AGENTS.md)
  - [Quick Reference](../firmware/esp32-c3-hub/QUICK-REFERENCE.md)

## Hardware
- [Hardware Inventory](hardware-inventory.md) - Complete parts list
- [Wiring Diagrams](wiring-diagrams/) - Schematics and pinouts
- [Sensor Datasheets](sensor-datasheets/) - Component specifications
- [Power Budget](power-budget.md) - Power consumption analysis

## Standards & Protocols
- [STANAG 4609 Implementation](stanag-4609-implementation.md) - MISB ST 0601 KLV
- [Oculus WebXR Teleop](oculus-webxr-teleop.md) - Quest 2/3/Pro control

## Ground Station
- [MediaMTX Config](ground-station/mediamtx/) - Video ingest + WebRTC
- [WebXR App](ground-station/webxr-app/) - Three.js + WebXR teleop

## Testing
- [Unit Tests](tests/unit/)
- [Integration Tests](tests/integration/)

## Tools
- [Calibration](tools/calibration/) - Sensor calibration scripts
- [Flashing](tools/flashing/) - Firmware upload utilities
- [Monitoring](tools/monitoring/) - Telemetry visualization

## Meta
- [AI Agent Instructions (Root)](../AGENTS.md)
- [Quick Reference (Root)](../QUICK-REFERENCE.md)
- [Contributing](../CONTRIBUTING.md)
- [Changelog](../CHANGELOG.md)
- [Dev Container Setup](DEV_CONTAINER.md)