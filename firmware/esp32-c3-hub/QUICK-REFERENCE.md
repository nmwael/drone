# Quick Reference — ESP32-C3 Sensor Hub Firmware

## ⚠️ Status: PLANNED / SCAFFOLD — Partial Build Exists

---

## 🎯 Target Specs

| Parameter | Value |
|-----------|-------|
| MCU | ESP32-C3 (RISC-V, 400KB SRAM) |
| Framework | ESP-IDF v5.5+ |
| GPS | u-blox ZED-F9P (RTK, UART + PPS) |
| IMU | ICM-20948 (9-DoF, I2C) @ 100 Hz |
| Magnetometer | QMC5883L / RM3100 (I2C) @ 50 Hz |
| Barometer | BMP280 (I2C) @ 10 Hz |
| Temp/Humidity | AHT20 (I2C) @ 1 Hz |
| Sensor Fusion | EKF (15+ state) @ 100 Hz |
| ESP-NOW | 2.4 GHz broadcast to S3 |
| UART to S3 | GPIO 17/18 @ 115200 baud |
| UART to RP2040s | 3× UART @ 115200 baud |

---

## 📁 Project Structure

```
esp32-c3-hub/
├── CMakeLists.txt              # Minimal (project only)
├── sdkconfig.defaults          # ESP-NOW config
├── partitions.csv              # Standard partitions
├── main/
│   ├── main.c                  # Entry, init sequence, health loop
│   ├── gps_gnss.c/h            # ZED-F9P UART + PPS
│   ├── magnetometer.c/h        # Mag I2C
│   ├── imu_baro.c/h            # ICM-20948 + BMP280 I2C
│   ├── aht20.c/h               # Temp/Humidity I2C
│   ├── bmp280.c/h              # Baro Pressure/Temp I2C
│   ├── sensor_fusion_ekf.c/h   # EKF fusion (15+ state)
│   ├── comms_espnow.c/h        # ESP-NOW to S3
│   ├── comms_uart_rp2040.c/h   # UART to 3× RP2040 ToF
│   └── telemetry_provider.c/h  # UART data source for S3
└── build/                      # Existing build artifacts
```

---

## ⚡ Commands

```bash
cd /workspaces/drone/firmware/esp32-c3-hub

# Setup
idf.py set-target esp32c3
idf.py menuconfig

# Build (artifacts exist)
idf.py build

# Flash & Monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Clean build
idf.py fullclean && idf.py build
```

---

## 🔑 Key SDK Config (sdkconfig.defaults)

```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_ESP_WIFI_ESPNOW=y
CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM=6
CONFIG_ESP_WIFI_ESPNOW_ENABLE_LONG_RANGE=y
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
```

---

## 🔌 Hardware Connections (Planned)

| Interface | GPIO | Connected To |
|-----------|------|--------------|
| I2C0 SDA/SCL | 11/12 | IMU, Mag, Baro, AHT20, BMP280 |
| UART0 RX/TX | 20/21 | GPS ZED-F9P + PPS |
| UART1 RX/TX | 17/18 | ESP32-S3 Vision (telemetry) |
| UART2 RX/TX | 6/7 | RP2040 ToF #1 |
| UART3 RX/TX | 8/9 | RP2040 ToF #2 |
| UART4 RX/TX | 10/11 | RP2040 ToF #3 |
| ESP-NOW | WiFi Radio | S3 Vision (2.4 GHz) |

---

## 🧩 Component APIs (To Be Defined)

| Component | Key Functions |
|-----------|---------------|
| `gps_gnss` | `init()`, `read()`, `get_pps_time()` |
| `magnetometer` | `init()`, `read_heading()`, `read_raw()` |
| `imu_baro` | `init()`, `read_imu()`, `read_baro()` |
| `aht20` | `init()`, `read_temp_humidity()` |
| `bmp280` | `init()`, `read_pressure_temp()` |
| `sensor_fusion_ekf` | `init()`, `update(imu)`, `update_gps()`, `get_state()` |
| `comms_espnow` | `init()`, `send_telemetry()`, `set_peer()` |
| `comms_uart_rp2040` | `init()`, `send_cmd()`, `recv_frame()` |
| `telemetry_provider` | `init()`, `send_to_s3()`, `recv_from_s3()` |

---

## 📊 Sensor Rates & EKF

| Source | Rate | EKF Role |
|--------|------|----------|
| IMU (accel/gyro) | 100 Hz | Propagation (predict) |
| GPS pos/vel | 10 Hz | Measurement update |
| Magnetometer | 50 Hz | Yaw correction |
| Barometer | 10 Hz | Altitude correction |
| PPS (GPS) | 1 Hz | Time sync |
| EKF Output | 100 Hz | Fused pose + covariance |

---

## 🔧 EKF State Vector (Planned, 19 States)

```
Position NED          : 3  (m)
Velocity NED          : 3  (m/s)
Attitude Quaternion   : 4  (w, x, y, z)
Accel Bias            : 3  (m/s²)
Gyro Bias             : 3  (rad/s)
Mag Bias (optional)   : 3  (µT)
──────────────────────────────
Total                 : 19 states
```

---

## 📋 Implementation Checklist

- [ ] Create all `.h` headers with API definitions
- [ ] Implement ZED-F9P GPS driver (UART + PPS)
- [ ] Implement ICM-20948 IMU driver (I2C, 100 Hz)
- [ ] Implement RM3100/QMC5883L magnetometer (I2C)
- [ ] Implement BMP280 barometer (I2C)
- [ ] Implement AHT20 temp/humidity (I2C)
- [ ] Implement 19-state EKF fusion (or port existing)
- [ ] Implement telemetry_provider (UART protocol to S3)
- [ ] Implement ESP-NOW broadcast to S3
- [ ] Implement 3× UART protocol to RP2040 ToF boards
- [ ] Add unit tests (Unity, like simulator)
- [ ] Create documentation (API.md, BUILD.md, TESTING.md, etc.)

---

## 🔗 Reference: STANAG S3 Simulator

| Simulator File | Hub Adaptation |
|----------------|----------------|
| `../stanag-s3-simulator/main/sensor_sim.c` | → Real sensor drivers + EKF |
| `../stanag-s3-simulator/main/uart_telemetry.c` | → `telemetry_provider.c` (bidirectional) |
| `../stanag-s3-simulator/main/network_tx.c` | → `comms_espnow.c` (ESP-NOW) |
| `../stanag-s3-simulator/main/klv_builder.c` | → KLV tags for S3 vision consumption |
| `../stanag-s3-simulator/test/` | → Unity test patterns |

---

## 📚 Key References

- **u-blox ZED-F9P:** https://www.u-blox.com/en/product/zed-f9p
- **ICM-20948:** https://invensense.tdk.com/products/motion-tracking/9-axis/icm-20948/
- **RM3100:** https://www.pni.com/products/rm3100/
- **BMP280:** https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp280/
- **AHT20:** https://www.asairstd.com/en/product/aht20.html
- **ESP-IDF ESP-NOW:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/network/esp_now.html
- **EKF References:** PX4 EKF2, ArduPilot AP_NavEKF, ROS robot_localization