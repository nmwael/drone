# Shopping List - RC Monster Truck Sensor Suite

**Status Legend:**
- ✅ **ON HAND** - Already owned
- 🛒 **TO BUY** - Need to purchase
- 🔄 **SALVAGED** - From Honeybee CP2

---

## Microcontrollers

| Item | Qty | Status | Est. Cost | Notes / Links |
|------|-----|--------|-----------|---------------|
| ESP32-S3 DevKit (with PSRAM, 8MB) | 1 | ✅ ON HAND | $0 | Must have PSRAM for H.264 encoding |
| ESP32-C3 DevKit / SuperMini | 1 | ✅ ON HAND | $0 | Sensor hub |
| RP2040 (Pi Pico or bare module) | 3 | ✅ ON HAND | $0 | Motor, Steering+ToF, Rear ToF |
| ESP8266MOD (AI Thinker) | 1 | ✅ ON HAND | $0 | Backup telemetry / watchdog / OTA |

---

## Vision Sensors

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **Thermal Camera** | 1 | 🛒 TO BUY | **$45-65** | **MLX90640 32×24** (Melexis) - I2C, 32Hz, -40°C to 300°C<br>→ [DigiKey](https://www.digikey.com/en/products/detail/melexis/MLX90640ESF-BAA-000-TU/11775055) / [Mouser](https://www.mouser.com/ProductDetail/Melexis/MLX90640ESF-BAA-000-TU) |
| **IR Camera (Night Vision)** | 1 | 🛒 TO BUY | **$15-25** | **OV5640 5MP Camera Module** (CSI/Parallel, IR filter removed)<br>→ [Arducam OV5640](https://www.arducam.com/product/ov5640-5mp-camera-module/) / [Amazon](https://www.amazon.com/s?k=OV5640+camera+module) |
| **Thermal Camera Alt (lower res)** | - | - | $25-35 | AMG8833 8×8 (Panasonic) - cheaper but low res |

---

## Navigation Sensors

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **GNSS/GPS (RTK)** | 1 | 🛒 TO BUY | **$150-250** | **u-blox ZED-F9P** (RTK, dual-band, 1cm accuracy)<br>→ [SparkFun](https://www.sparkfun.com/products/16481) / [DigiKey](https://www.digikey.com/en/products/detail/u-blox/ZED-F9P-00B/11677343) |
| GPS Antenna (Survey grade) | 1 | 🛒 TO BUY | $30-50 | u-blox ANN-MB-00 or Taoglas AA.161 |
| **Magnetometer** | 1 | 🛒 TO BUY | **$8-15** | **QMC5883L** (cheap, I2C, 16-bit)<br>→ [Amazon](https://www.amazon.com/s?k=QMC5883L) / [AliExpress](https://www.aliexpress.com/wholesale?SearchText=QMC5883L) |
| **IMU (Accel + Gyro)** | 1 | 🛒 TO BUY | **$12-20** | **ICM-20948** (9-DoF, DMP, I2C/SPI)<br>→ [SparkFun](https://www.sparkfun.com/products/16485) / [DigiKey](https://www.digikey.com/en/products/detail/tdk-invensense/ICM-20948/8708445) |
| **Barometer / Pressure** | 1 | ✅ ON HAND | $0 | **BMP280** (I2C, 0x76/0x77) |
| **Temperature / Humidity** | 1 | ✅ ON HAND | $0 | **AHT20** (I2C, 0x38) |

---

## Obstacle Avoidance (ToF Array)

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **VL53L5CX 8×8 Zone ToF** | 4 | 🛒 TO BUY | **$18-25 ea ($72-100 total)** | STMicroelectronics - 8×8 zones, 4m range, I2C @ 1MHz<br>→ [DigiKey](https://www.digikey.com/en/products/detail/stmicroelectronics/VL53L5CXV0DH/1/12849648) / [Mouser](https://www.mouser.com/ProductDetail/STMicroelectronics/VL53L5CXV0DH) |
| XSHUT Level Shifter / Mux | 1 | 🛒 TO BUY | $5-10 | 4-channel I2C mux (TCA9548A) or GPIO XSHUT control |

---

## Power & Actuation

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **Motor Driver (Dual H-Bridge)** | 1 | 🛒 TO BUY | **$15-35** | **BTS7960** (43A peak, 5-28V) or **VNH5019** (30A, 5.5-24V)<br>→ [Amazon BTS7960](https://www.amazon.com/s?k=BTS7960) / [Pololu VNH5019](https://www.pololu.com/product/2807) |
| **Servo (Steering)** | 1 | 🔄 SALVAGED | $0 | From Honeybee CP2 (2x main rotor servos, ~1.5kg-cm) |
| **Servo (Camera Gimbal - Future)** | 1 | 🔄 SALVAGED | $0 | 2nd CP2 servo |
| **3.3V Regulator (3A+)** | 1 | 🛒 TO BUY | $8-15 | **TPS5430** (3A buck) or **LM2596S-3.3** with heatsink |
| **5V BEC (3A+)** | 1 | 🛒 TO BUY | $8-15 | **LM2596S-5.0** or UBEC 5V/3A |
| **Power Distribution Board** | 1 | 🛒 TO BUY | $10-20 | Custom PCB or perfboard with XT60/XT30 connectors |
| **Main Battery** | 1 | 🛒 TO BUY | $30-60 | **2S LiPo 5000-8000mAh 50C+** (7.4V) or **3S 5000mAh** (11.1V)<br>→ [Gens Ace](https://www.gensace.de/) / [HRB](https://www.hrb-battery.com/) |
| **Battery Charger** | 1 | 🔄 SALVAGED | $0 | CP2 charger (1S/2S) - may need upgrade for 2S/3S |

---

## Communications & Ground Station

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **WiFi 6 Router / AP** | 1 | 🛒 TO BUY | **$80-150** | **ASUS RT-AX58U** or **GL.iNet GL-AXT1800** (travel router, OpenWrt)<br>5 GHz, 80 MHz channel, low latency |
| **External Antenna (ESP32-S3)** | 1 | 🛒 TO BUY | $10-20 | 2.4/5 GHz dual-band PCB antenna or u.FL → SMA pigtail |
| **Ground Station PC / Raspberry Pi 5** | 1 | 🛒 TO BUY | **$0 (PC) / $80 (Pi 5)** | PC preferred for WebXR; Pi 5 can run MediaMTX only |
| **Meta Quest 2/3/Pro** | 1 | 🛒 TO BUY | **$300-500** | Quest 3 recommended (better passthrough, resolution) |

---

## Mechanical / Chassis

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| RC Monster Truck Chassis | 1 | 🛒 TO BUY | **$100-300** | 1/10 scale, 4WD, brushless preferred (e.g., Arrma, Traxxas, HSP) |
| Camera Mount / Gimbal | 1 | 🛒 TO BUY | $20-50 | 2-axis (pan/tilt) for OV5640 + MLX90640 |
| Vibration Dampening | 1 | 🛒 TO BUY | $10-20 | Silicone grommets, foam tape for IMU/camera isolation |
| Wiring / Connectors | 1 | 🛒 TO BUY | $20-40 | JST-SH, JST-GH, DuPont, XT60, heat shrink, cable sleeving |

---

## Development Tools

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| USB-UART Adapter (CP2102/CH340) | 2-3 | 🛒 TO BUY | $5-10 ea | For flashing ESP32-C3/S3 and RP2040s |
| JTAG Debugger (ESP-Prog / J-Link) | 1 | 🛒 TO BUY | $25-60 | ESP-Prog ($25) for ESP32 debugging |
| Logic Analyzer (Saleae / DSLogic) | 1 | 🛒 TO BUY | $50-150 | I2C/SPI/UART debugging |
| Multimeter / Oscilloscope | 1 | ✅ ON HAND | $0 | Basic bench tools |

---

## Cost Summary

| Category | Est. Cost (Min) | Est. Cost (Max) |
|----------|----------------|----------------|
| Vision Sensors | $60 | $90 |
| Navigation Sensors | $200 | $305 |
| ToF Array (4×) | $72 | $100 |
| Power & Actuation | $71 | $145 |
| Comms & Ground Station | $390 | $720 |
| Mechanical | $130 | $410 |
| Dev Tools | $80 | $220 |
| **TOTAL** | **~$1,003** | **~$1,990** |

> **Notes:**
> - **Min cost** assumes: Quest 2 used, Pi 5 ground station, basic truck chassis, AliExpress/Direct from China for sensors
> - **Max cost** assumes: Quest 3 new, PC ground station, quality chassis, DigiKey/Mouser for all parts
> - **Already owned** saves ~$100-150 (MCUs, BMP280, AHT20, CP2 servos/motors/charger, basic tools)

---

## Priority Order (Build Phases)

### Phase 1: Core Navigation (Week 1-2) 🚨 **START HERE**
1. u-blox ZED-F9P + antenna ($180)
2. QMC5883L ($10)
3. ICM-20948 ($15)
4. BMP280 + AHT20 ✅ ON HAND
5. ESP32-C3 firmware: GPS + Mag + IMU + EKF

### Phase 2: Motor Control & RC Fallback (Week 2-3)
1. BTS7960 / VNH5019 driver ($25)
2. 3.3V/5V regulators ($20)
3. Power distribution board ($15)
4. RP2040#1 firmware: PID + RC PWM input

### Phase 3: Obstacle Avoidance (Week 3-4)
1. 4× VL53L5CX ($80-100)
2. TCA9548A I2C mux ($5)
3. RP2040#2 & #3 firmware: ToF scanning

### Phase 4: Vision & STANAG (Week 4-6)
1. MLX90640 thermal ($55)
2. OV5640 camera ($20)
3. ESP32-S3 with PSRAM ✅ ON HAND
4. H.264 + KLV + TS firmware

### Phase 5: Ground Station & Oculus (Week 6-8)
1. WiFi 6 router ($100)
2. Meta Quest 3 ($500) or Quest 2 used ($300)
3. MediaMTX + WebXR app development

### Phase 6: Integration & Field Test (Week 8+)
1. Truck chassis ($150-300)
2. Camera gimbal mount ($30)
3. Vibration isolation ($15)
4. Full system integration

---

## Supplier Quick Links

| Supplier | Regions | Notes |
|----------|---------|-------|
| **DigiKey** | Global | Fast shipping, genuine parts, higher price |
| **Mouser** | Global | Good stock, genuine parts |
| **SparkFun** | US/Global | Breakout boards, good docs |
| **Adafruit** | US/Global | Breakout boards, tutorials |
| **AliExpress** | Global | Cheapest, 2-4 week shipping, verify seller |
| **Amazon** | Global | Fast, but markup on components |
| **Pololu** | US/Global | Robotics-focused, good motor drivers |
| **Banggood** | Global | RC trucks, batteries, cheap modules |
| **HobbyKing** | Global | LiPo batteries, RC gear |

---

## Part Numbers for Quick Search

```
MLX90640ESF-BAA-000-TU     (Thermal 32x24)
OV5640-CAM                 (5MP Camera)
ZED-F9P-00B                (u-blox RTK GPS)
ANN-MB-00                  (GPS Antenna)
QMC5883L                   (Magnetometer)
ICM-20948                  (9-DoF IMU)
BMP280                     (Pressure - HAVE)
AHT20                      (Temp/Humidity - HAVE)
VL53L5CXV0DH               (8x8 ToF x4)
BTS7960                    (43A Motor Driver)
TPS5430                    (3A Buck Reg)
LM2596S-3.3 / LM2596S-5.0  (LDO/Buck)
TCA9548A                   (I2C Mux)
RT-AX58U                   (WiFi 6 Router)
```

---

**Next Action:** Order Phase 1 parts (GPS, Mag, IMU) to start ESP32-C3 sensor hub firmware development.