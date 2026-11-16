# AI Agent Instructions — ESP32-C3 Sensor Hub Firmware

## Quick Context

**Status:** 🚧 **Planned / Scaffold** — Partial build exists (has build artifacts)  
**Target:** ESP32-C3 (RISC-V, 400KB SRAM, 4MB Flash)  
**Framework:** ESP-IDF v5.5+  
**Language:** C (C99)  
**Build:** `idf.py` (CMake + Ninja)  

**Scope:** GPS + IMU + Mag + Baro + Temp/RH → EKF Fusion → Telemetry to S3 Vision. **No RP2040 ToF boards.**

---

## Planned Architecture

```
Sensors (I2C/UART) → Raw Data → EKF Fusion → Fused Pose (100 Hz)
                                                    ↓
                    ┌──────────────────────────────┼──────────────────────────────┐
                    ▼                              ▼                              ▼
         Telemetry Provider (UART)         ESP-NOW Broadcast              (Available for
         → ESP32-S3 Vision                 → ESP32-S3 Vision               future expansion)
         @ 115200 baud                     @ 2.4 GHz
```

---

## Key Components (from main.c & CMakeLists.txt)

| Component | Header | Purpose |
|-----------|--------|---------|
| GPS/GNSS | `gps_gnss.h` | u-blox ZED-F9P via UART + PPS |
| Magnetometer | `magnetometer.h` | QMC5883L/RM3100 via I2C |
| IMU + Baro | `imu_baro.h` | ICM-20948 (9-DoF) + BMP280 via I2C |
| AHT20 | `aht20.h` | Temp/Humidity via I2C |
| BMP280 | `bmp280.h` | Baro Pressure/Temp via I2C |
| EKF Fusion | `sensor_fusion_ekf.h` | 15+ state Extended Kalman Filter |
| ESP-NOW Comms | `comms_espnow.h` | Broadcast to ESP32-S3 |
| Telemetry Provider | `telemetry_provider.h` | Data source for S3 vision |

**Removed:** `comms_uart_rp2040.h` (RP2040 ToF no longer planned)

---

## Sensor Suite & Rates

| Sensor | Interface | Rate | Purpose |
|--------|-----------|------|---------|
| **GPS (ZED-F9P)** | UART + PPS | 10 Hz | Position, velocity, time (RTK) |
| **IMU (ICM-20948)** | I2C | 100 Hz | Accel, gyro, mag (9-DoF) |
| **Magnetometer** | I2C | 50 Hz | Precision heading (RM3100) |
| **Barometer (BMP280)** | I2C | 10 Hz | Pressure, altitude |
| **AHT20** | I2C | 1 Hz | Cabin temp/humidity |
| **EKF Output** | Internal | 100 Hz | Fused pose + covariance |

---

## Planned Data Structures (to be defined)

```c
// Raw sensor readings
typedef struct {
    // GPS
    double lat, lon, alt;       // WGS-84
    float vn, ve, vd;           // NED velocity (m/s)
    uint32_t tow_ms;            // GPS time of week
    uint8_t fix_type, num_sv;   // RTK fix quality
    
    // IMU
    float ax, ay, az;           // Accel (m/s²)
    float gx, gy, gz;           // Gyro (rad/s)
    float mx, my, mz;           // Mag (µT)
    
    // Barometer
    float pressure;             // hPa
    float baro_alt;             // m
    float baro_temp;            // °C
    
    // AHT20
    float temp, humidity;       // °C, %RH
    
    uint64_t timestamp_us;      // Common timestamp
} raw_sensor_data_t;

// EKF fused state
typedef struct {
    float pos_ned[3];           // NED position (m)
    float vel_ned[3];           // NED velocity (m/s)
    float quat[4];              // Attitude quaternion (w,x,y,z)
    float euler[3];             // Roll, pitch, yaw (rad)
    float pos_cov[9];           // Position covariance (3x3)
    float vel_cov[9];           // Velocity covariance
    float att_cov[9];           // Attitude covariance
    bool converged;             // EKF convergence flag
    uint64_t timestamp_us;
} ekf_state_t;

// Telemetry to S3 vision (UART/ESP-NOW)
typedef struct {
    ekf_state_t pose;
    raw_sensor_data_t sensors;
    uint32_t sensor_health;     // Bitmask: GPS|IMU|Mag|Baro|AHT20
    float cpu_load;             // FreeRTOS CPU usage
    uint64_t timestamp_us;
} hub_telemetry_t;
```

---

## EKF Fusion Design (Planned)

### State Vector (15+ states)
```
x = [pos_N, pos_E, pos_D,      // 3 - Position (NED)
     vel_N, vel_E, vel_D,       // 3 - Velocity (NED)
     q_w, q_x, q_y, q_z,        // 4 - Attitude quaternion
     acc_bias_x, acc_bias_y, acc_bias_z,  // 3 - Accel bias
     gyro_bias_x, gyro_bias_y, gyro_bias_z,  // 3 - Gyro bias
     mag_bias_x, mag_bias_y, mag_bias_z]  // 3 - Mag bias (optional)
Total: 19 states
```

### Process Model
- **IMU propagation** @ 100 Hz (accel + gyro integration)
- **Bias random walk** for accel/gyro
- **Quaternion kinematics** for attitude

### Measurement Updates
| Measurement | Rate | States Updated |
|-------------|------|----------------|
| GPS pos/vel | 10 Hz | Position, velocity |
| Mag heading | 50 Hz | Yaw (q_z) |
| Baro altitude | 10 Hz | Position D |
| PPS time sync | 1 Hz | Clock bias |

---

## Common AI Tasks (When Implemented)

### 1. Implementing I2C Sensor Drivers
```c
// Pattern for each sensor (gps_gnss.c, imu_baro.c, etc.)
esp_err_t sensor_init(void) {
    // I2C/UART config, register setup, self-test
    return ESP_OK;
}

void sensor_read(raw_sensor_data_t *out) {
    // Read registers, convert to SI units, populate struct
}

// Register in main.c initialization sequence
```

### 2. EKF Implementation Options
- **Custom EKF**: Full control, implement in `sensor_fusion_ekf.c`
- **ESP-IDF `esp_ekf`**: If available in v5.5+
- **Port from PX4/ArduPilot**: Reference implementations exist

### 3. Telemetry Provider Protocol (to S3 Vision)
```c
// UART frame format (example)
typedef struct {
    uint8_t sync[2];        // 0xAA 0x55
    uint16_t msg_id;        // Message type
    uint16_t len;           // Payload length
    uint8_t payload[];      // JSON or protobuf
    uint16_t crc;           // CRC16
} uart_frame_t;

// ESP-NOW payload (broadcast)
typedef struct {
    uint8_t version;
    hub_telemetry_t telemetry;
    uint32_t crc;
} espnow_frame_t;
```

---

## Build Configuration (Planned)

### Required `sdkconfig.defaults`
```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_ESP_WIFI_ESPNOW=y
CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM=6
CONFIG_ESP_WIFI_ESPNOW_ENABLE_LONG_RANGE=y
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
```

### Partition Table (minimal)
```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x100000,
```

---

## Hardware Connections (Planned)

| Interface | ESP32-C3 GPIO | Connected To |
|-----------|---------------|--------------|
| I2C0 SDA/SCL | GPIO 11/12 | IMU, Mag, Baro, AHT20, BMP280 |
| UART0 RX/TX | GPIO 20/21 | GPS ZED-F9P + PPS |
| UART1 RX/TX | GPIO 17/18 | ESP32-S3 Vision (telemetry) |
| WiFi/ESP-NOW | Internal | ESP32-S3 Vision (2.4 GHz) |

**Removed:** UART2/3/4 for RP2040 ToF boards

---

## Testing Strategy (Planned)

| Test | Approach |
|------|----------|
| Sensor drivers | Unit tests with mocked I2C/UART |
| EKF fusion | Synthetic data with known ground truth |
| Telemetry output | Capture UART, validate JSON/protobuf |
| ESP-NOW latency | Measure S3 receive timestamp |
| Sensor health | Simulate failures, verify detection |

---

## Reference: STANAG S3 Simulator

| Simulator Component | Hub Adaptation |
|---------------------|----------------|
| `sensor_sim.c` | → Real sensor drivers + EKF |
| `uart_telemetry.c` | → `telemetry_provider.c` (bidirectional) |
| `network_tx.c` | → `comms_espnow.c` (ESP-NOW broadcast) |
| `main.c` init sequence | → Similar multi-sensor init |

---

## When in Doubt

1. **Check simulator** — `../stanag-s3-simulator/main/` for patterns
2. **ESP-IDF examples** — `peripherals/i2c`, `peripherals/uart`, `wifi/espnow`
3. **EKF references** — PX4 `EKF2`, ArduPilot `AP_NavEKF`, Robot Localization ROS pkg
4. **Sensor datasheets** — u-blox ZED-F9P, ICM-20948, RM3100, BMP280, AHT20