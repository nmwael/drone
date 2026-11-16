# ESP32-C3 Sensor Hub Firmware — GPS + IMU + Mag + EKF Fusion

**Status:** 🚧 **Planned / Scaffold** — Partial build exists (has build artifacts)

Sensor fusion hub for ESP32-C3: GPS/GNSS, 9-DoF IMU, Magnetometer, Barometer, Temperature/Humidity. Runs Extended Kalman Filter (EKF) for fused navigation solution. Provides telemetry to ESP32-S3 vision via ESP-NOW and UART.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-C3 (Sensor Hub)                      │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │  GPS/    │ │ Magnet-  │ │  IMU +   │ │  AHT20   │ │ BMP280 │ │
│  │  GNSS    │ │  ometer  │ │  Baro    │ │ (Temp/RH)│ │(Press) │ │
│  │ (UART)   │ │  (I2C)   │ │  (I2C)   │ │  (I2C)   │ │ (I2C)  │ │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬───┘ │
│       │            │            │            │            │     │
│       ▼            ▼            ▼            ▼            ▼     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Sensor Fusion EKF                            │  │
│  │  GPS + IMU + Mag + Baro → Fused Pose (pos/vel/att)       │  │
│  └──────────────────────────┬────────────────────────────────┘  │
│                             │                                   │
│              ┌──────────────┼──────────────┐                    │
│              ▼              ▼              ▼                    │
│  ┌─────────────────┐ ┌──────────────┐ ┌─────────────────────┐  │
│  │ Telemetry       │ │ ESP-NOW to   │ │ (Available for      │  │
│  │ Provider (S3)   │ │ S3 Vision    │ │ future expansion)  │  │
│  │ (UART @ 115200) │ │ (2.4 GHz)    │ │                     │  │
│  └─────────────────┘ └──────────────┘ └─────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Planned Components (from CMakeLists.txt & main.c)

| Component | Header | Purpose |
|-----------|--------|---------|
| **GPS/GNSS** | `gps_gnss.h` | u-blox ZED-F9P (RTK) via UART |
| **Magnetometer** | `magnetometer.h` | QMC5883L / HMC5883L / RM3100 (I2C) |
| **IMU + Baro** | `imu_baro.h` | ICM-20948 (IMU) + BMP280 (I2C) |
| **AHT20** | `aht20.h` | Temperature/Humidity (I2C) |
| **BMP280** | `bmp280.h` | Barometric Pressure/Temperature (I2C) |
| **EKF Fusion** | `sensor_fusion_ekf.h` | Extended Kalman Filter for nav |
| **ESP-NOW Comms** | `comms_espnow.h` | ESP-NOW to ESP32-S3 vision |
| **Telemetry Provider** | `telemetry_provider.h` | Data source for S3 vision (UART) |

**Removed:** UART to RP2040 ToF boards (no longer planned)

---

## Target Specifications

| Parameter | Target |
|-----------|--------|
| **MCU** | ESP32-C3 (RISC-V, 400KB SRAM) |
| **Framework** | ESP-IDF v5.5+ |
| **GPS** | u-blox ZED-F9P (RTK, PPS output) |
| **IMU** | ICM-20948 (9-DoF: 3-axis accel + gyro + mag) |
| **Magnetometer** | QMC5883L / RM3100 (dedicated, high precision) |
| **Barometer** | BMP280 / BMP390 (I2C) |
| **Temp/Humidity** | AHT20 (I2C) |
| **Sensor Fusion** | EKF (15+ state: pos/vel/att/bias) |
| **Output Rate** | 100 Hz (IMU), 10 Hz (GPS/Mag/Baro), 1 Hz (AHT20) |
| **ESP-NOW** | 2.4 GHz, broadcast to S3 vision |
| **UART to S3** | GPIO 17/18, 115200 baud, JSON/protobuf |
| **PPS Input** | GPIO for GPS time sync |

---

## Key Features

### Sensor Fusion EKF
- **State Vector**: Position (3), Velocity (3), Attitude (4 quat), IMU biases (6), Mag bias (3)
- **Measurements**: GPS pos/vel, IMU accel/gyro, Mag heading, Baro altitude
- **Output**: Fused pose @ 100 Hz for S3 vision timestamp alignment
- **Convergence**: Monitor via `sensor_fusion_ekf_is_converged()`

### Telemetry Provider for S3 Vision
- Aggregates fused pose + raw sensor data
- Outputs via UART @ 115200 to S3 vision
- Format: JSON or binary protobuf (TBD)
- Includes: Position, velocity, attitude, covariance, sensor health

### ESP-NOW to S3 Vision
- Low-latency telemetry broadcast
- Fallback if UART congested
- Includes: Fused pose, sensor health, alerts

---

## Build & Development

```bash
cd /workspaces/drone/firmware/esp32-c3-hub

# Set target (once)
idf.py set-target esp32c3

# Configure
idf.py menuconfig

# Build (already has build artifacts)
idf.py build

# Flash & monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### Required SDK Config
```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_ESP_WIFI_ESPNOW=y
CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM=6
CONFIG_ESP_WIFI_ESPNOW_ENABLE_LONG_RANGE=y
```

---

## Hardware Connections (Planned)

| Interface | GPIO | Purpose |
|-----------|------|---------|
| **I2C0** | GPIO 11/12 | IMU, Mag, Baro, AHT20, BMP280 |
| **UART0** | GPIO 20/21 | GPS/GNSS (ZED-F9P) + PPS |
| **UART1** | GPIO 17/18 | ESP32-S3 Vision (telemetry) |
| **ESP-NOW** | Internal WiFi | 2.4 GHz to S3 |
| **Status LED** | GPIO 8 (or similar) | Build-dependent |

**Removed:** UART2/3/4 for RP2040 ToF boards

---

## Development Status

| Component | Status | Notes |
|-----------|--------|-------|
| Project scaffold | ✅ CMakeLists.txt, sdkconfig | Build artifacts exist |
| Main entry | ✅ main.c | Task structure outlined |
| GPS/GNSS | 🚧 Placeholder | `gps_gnss.h` declared |
| Magnetometer | 🚧 Placeholder | `magnetometer.h` declared |
| IMU + Baro | 🚧 Placeholder | `imu_baro.h` declared |
| AHT20 | 🚧 Placeholder | `aht20.h` declared |
| BMP280 | 🚧 Placeholder | `bmp280.h` declared |
| EKF Fusion | 🚧 Placeholder | `sensor_fusion_ekf.h` declared |
| ESP-NOW Comms | 🚧 Placeholder | `comms_espnow.h` declared |
| Telemetry Provider | 🚧 Placeholder | `telemetry_provider.h` declared |

**Removed:** `comms_uart_rp2040.h` (RP2040 ToF no longer planned)

---

## Next Implementation Steps

1. **Create component headers** (`.h` files) with API definitions
2. **Implement I2C sensor drivers** — GPS UART, IMU, Mag, Baro, AHT20, BMP280
3. **Implement EKF fusion** — 15+ state EKF (or use ESP-IDF `esp_ekf` if available)
4. **Implement telemetry provider** — UART protocol to S3 vision
5. **Implement ESP-NOW comms** — Broadcast to S3 vision
6. **Add unit tests** — Unity framework (like simulator)
7. **Create documentation** — API.md, BUILD.md, TESTING.md, etc.

---

## Related Documentation

| Document | Location |
|----------|----------|
| **AI Onboarding** | `AGENTS.md` (this directory) |
| **Quick Reference** | `QUICK-REFERENCE.md` (this directory) |
| **Root Project Docs** | `/workspaces/drone/AGENTS.md`, `QUICK-REFERENCE.md` |
| **ESP32-S3 Vision** | `../esp32-s3-vision/` (consumer of telemetry) |
| **STANAG S3 Simulator** | `../stanag-s3-simulator/` (reference for KLV/TS) |

---

## License

MIT — Same as parent project.