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

### Sensors (to procure/integrate)

| Sensor | Interface | Purpose | Candidate Parts |
|--------|-----------|---------|-----------------|
| **Thermal Camera** | I2C/SPI | Heat detection (8×8 to 32×32) | MLX90640 (32×24), AMG8833 (8×8), MLX90641 (16×12) |
| **IR Distance/Proximity** | I2C/UART | Obstacle avoidance | VL53L1X (ToF, 4m), VL53L5CX (8×8 zone), TF-Luna (LiDAR) |
| **IR Camera (Night Vision)** | CSI/Parallel | Low-light imaging | OV2640/OV5640 with IR filter removed, ESP32-S3 CSI |
| **GNSS/GPS** | UART/I2C | Positioning | u-blox ZED-F9P (RTK), BN-880, SAM-M10Q |
| **Magnetometer (Compass)** | I2C | Heading | QMC5883L, HMC5883L, RM3100, LIS3MDL |
| **IMU (Accel/Gyro)** | I2C/SPI | Orientation, vibration | ICM-20948, BMI270, MPU6050 |
| **Barometer** | I2C | Altitude | BMP390, MS5611 |

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

---

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
```

### Communication Bus

| Link | Protocol | Rate | Purpose |
|------|----------|------|---------|
| ESP32-S3 ↔ ESP32-C3 | ESP-NOW / UART | 1-2 Mbps | Sensor fusion, command/telemetry |
| ESP32-C3 ↔ RP2040s | UART / I2C | 1 Mbps / 400 kHz | Motor commands, sensor reads |
| RP2040 ↔ Motor Driver | PWM + GPIO | 20 kHz PWM | Motor speed/direction |
| RP2040 ↔ VL53L5CX | I2C (mux) | 1 MHz | 8×8 zone ToF scanning |
| **CP2 Receiver → RP2040** | **PWM / SBUS** | **50 Hz / 100 kbps** | **Manual RC fallback / override** |

### Control Modes

| Mode | Source | Path | Use Case |
|------|--------|------|----------|
| **Autonomous** | ESP32-S3/C3 | ESP-NOW → UART → RP2040 | Waypoint nav, obstacle avoidance |
| **Teleop (WiFi)** | Phone/PC | WiFi → ESP32-S3 → ESP-NOW → UART → RP2040 | FPV driving, sensor streaming |
| **RC Fallback** | CP2 Transmitter | **PWM/SBUS → RP2040** | **Safety override, testing, no-WiFi zones** |

> **Safety:** RP2040 firmware implements priority: RC PWM > Autonomous > Teleop. Loss of RC signal auto-switches to autonomous hold/return.

---

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
| GPIO 6/7 | I2C0 (Sensors) | GPS, Mag, IMU, Baro (mux if needed) |
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
| VL53L5CX ×4 | 4×20 | 3.3V | 264 |
| Motor Driver (idle) | 50 | 5V | 250 |
| Servo (moving) | 500 | 5V | 2500 |
| **Total (3.3V rail)** | ~1.2A | | **~4W** |
| **Total (5V rail, motors idle)** | ~0.55A | | **~2.75W** |
| **Peak (motors + servo)** | 10A+ | 7.4V | **~74W** |

> **Note:** Use separate 5V BEC (3A+) and 3.3V LDO (2A+) from main LiPo. Motor driver directly from LiPo.

---

## License

MIT License - See [LICENSE](LICENSE)