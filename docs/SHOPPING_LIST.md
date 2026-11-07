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
| RP2040 (Pi Pico or bare module) | 3 | ✅ ON HAND | $0 | Motor, Steering+ToF, Rear ToF — **16MB (128 Mbit) / 4MB (32 Mbit) Flash** |
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
| **VL53L5CX 8×8 Zone ToF** | **12** | 🛒 TO BUY | **$18-25 ea ($216-300 total)** | STMicroelectronics - 8×8 zones, 4m range, I2C @ 1MHz (4 per RP2040, 3 RP2040s)<br>→ [DigiKey](https://www.digikey.com/en/products/detail/stmicroelectronics/VL53L5CXV0DH/1/12849648) / [Mouser](https://www.mouser.com/ProductDetail/STMicroelectronics/VL53L5CXV0DH) |
| **TCA9548A I2C Mux** | **3** | 🛒 TO BUY | **$5-10 ea ($15-30 total)** | 8-channel I2C mux per RP2040 (4 ToF per mux)<br>→ [Amazon](https://www.amazon.com/s?k=TCA9548A) / [DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/TCA9548APWR/2963793) |

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

## Mechanical / Chassis — **Using Existing RC Monster Truck + CP2 Remote**

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| **RC Monster Truck** | 1 | ✅ **ON HAND** | **$0** | Using as-is with existing drivetrain, steering servo, ESC |
| **CP2 2.4GHz Remote** | 1 | ✅ **ON HAND** | **$0** | PWM/SBUS → RP2040 for RC fallback (already working) |
| **Camera Gimbal (2-axis)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - uses CP2 servos (pan/tilt for OV5640 + MLX90640) |
| **Sensor Enclosures** | 3-4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - C3 hub, motor driver, power distro, battery tray |
| **VL53L5CX Mounts** | 4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - front, rear, left, right brackets (truck-specific) |
| **Vibration Dampers** | 4-8 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - TPU Shore 60-80A for IMU/camera isolation |
| **GPS Antenna Mast** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - elevates ZED-F9P antenna above truck body |
| **Antenna Mount (ESP32-S3)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - u.FL/SMA external antenna bracket |
| **Wire Routing Clips** | 10+ | 🖨️ **PRINT** | **$0 (filament)** | Printable - cable management along chassis |
| **Servo Adapters / Horns** | 2-4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - adapt CP2 servos to steering/gimbal |
| **Battery Tray / Strap** | 1 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - secure 2S/3S LiPo to existing chassis |
| **ESC / Motor Driver Mount** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - mount BTS7960/VNH5019 to chassis |
| **Electronics Tray** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - mount ESP32-S3, C3, RP2040s to chassis |
| Wiring / Connectors | 1 | 🛒 TO BUY | $20-40 | JST-SH, JST-GH, DuPont, XT60, heat shrink, cable sleeving |
| Heat-set Inserts (M3/M4) | 50+ | 🛒 TO BUY | $10-15 | For strong bolt-on printed parts |
| M3/M4 Hardware (screws, nuts, standoffs) | 1 kit | 🛒 TO BUY | $15-25 | Brass standoffs, nylon lock nuts, button-head screws |

> **Design Approach:** Measure your truck's chassis hole patterns, body mount points, and servo spline type. Design printed parts in Fusion 360 / FreeCAD to bolt directly to existing mounting points. Use **TPU dampers** between electronics tray and chassis for vibration isolation.

---

## 3D Printing Filament (FDM Printer ✅ ON HAND)

| Material | Qty | Status | Use Case |
|----------|-----|--------|----------|
| **PETG** | On hand | ✅ **ON HAND** | Functional parts: enclosures, mounts, brackets (UV/heat resistant) |
| **TPU** | On hand | ✅ **ON HAND** | Vibration dampers, battery straps, flexible mounts (Shore 60-80A) |
| **PLA** | On hand | ✅ **ON HAND** | Prototypes, wire clips, non-structural test prints |
| **ASA** | 1 roll | 🛒 **OPTIONAL** | Outdoor/UV parts: GPS mast, antenna mounts ($30-40 if needed) |

> **Print Settings for Functional Parts:** 0.2mm layer, 3-4 walls, 25-30% gyroid infill, 220-240°C nozzle (PETG), 70-80°C bed. Dry filament 4h @ 65°C before printing PETG/TPU.

---

## Development Tools

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| **USB-UART Adapter** | 0 | ❌ **NOT NEEDED** | $0 | All boards have onboard USB (ESP32-S3, C3, RP2040, ESP8266) |
| JTAG Debugger (ESP-Prog / J-Link) | 1 | 🛒 TO BUY | $25-60 | ESP-Prog ($25) for ESP32 debugging |
| Logic Analyzer (Saleae / DSLogic) | 1 | 🛒 TO BUY | $50-150 | I2C/SPI/UART debugging |
| Multimeter / Oscilloscope | 1 | ✅ ON HAND | $0 | Basic bench tools |

---

## Cost Summary

| Category | Est. Cost (Min) | Est. Cost (Max) |
|----------|----------------|----------------|
| Vision Sensors | $60 | $90 |
| Navigation Sensors | $200 | $305 |
| ToF Array (12×) | $216 | $300 |
| Power & Actuation | $71 | $145 |
| Comms & Ground Station | $390 | $720 |
| **Mechanical (Printed Fittings)** | **$10** | **$25** |
| Dev Tools | **$75** | **$210** |
| **ASA Filament (optional)** | **$0** | **$40** |
| **TOTAL** | **~$1,022** | **~$1,835** |

> **Notes:**
> - **Min cost** assumes: Quest 2 used, Pi 5 ground station, AliExpress/Direct from China for sensors
> - **Max cost** assumes: Quest 3 new, PC ground station, DigiKey/Mouser for all parts
> - **Already owned saves ~$500-600**: MCUs, BMP280, AHT20, CP2 servos/motors/charger, **RC truck + remote**, basic tools, **FDM printer + PETG/TPU/PLA filament**
> - **3D Printing saves ~$150-400** vs buying enclosures, gimbal, mounts, dampers, battery trays (only hardware/inserts to buy)

---

## Priority Order (Build Phases)

### Phase 0: 3D Print Preparation (Week 0) 🖨️ **DO FIRST**
1. **Slice & print test coupons** - calibrate flow, temperature, bed adhesion
2. **Print vibration dampers (TPU)** - 4-8 pcs, 30min each, test Shore hardness
3. **Print sensor enclosures (PETG)** - C3 hub, power distro, electronics tray (4-6h each)
4. **Print VL53L5CX mounts (PETG)** - **12 pcs**, 1h each
5. **Print camera gimbal (PETG + TPU)** - 6-8h, uses CP2 servos
6. **Print GPS antenna mast, antenna mount, wire clips** - 2-3h total
7. **Order heat-set inserts (M3/M4)** - install in printed parts

> **Why Phase 0?** Parts take 20-30h total print time. Start printing while waiting for Phase 1 sensors to arrive.

### Phase 1: Core Navigation (Week 1-2) 🚨 **START HERE**
1. u-blox ZED-F9P + antenna ($180)
2. QMC5883L ($10)
3. ICM-20948 ($15)
4. BMP280 + AHT20 ✅ ON HAND
5. ESP32-C3 firmware: GPS + Mag + IMU + EKF
6. **Install sensors in printed C3 hub enclosure**

### Phase 2: Motor Control & RC Fallback (Week 2-3)
1. BTS7960 / VNH5019 driver ($25)
2. 3.3V/5V regulators ($20)
3. **Printed power distribution box** (already printed)
4. RP2040#1 firmware: PID + RC PWM input
5. **Install motor driver in printed enclosure**

### Phase 3: Obstacle Avoidance (Week 3-4)
1. **12× VL53L5CX** ($216-300)
2. **3× TCA9548A I2C mux** ($15-30)
3. **Printed VL53L5CX mounts** (already printed - 12 brackets)
4. RP2040 #1, #2, #3 firmware: 4× ToF scanning each (12 total)
5. **Mount ToF sensors on truck chassis** (front, rear, left, right, corners)

### Phase 4: Vision & STANAG (Week 4-6)
1. MLX90640 thermal ($55)
2. OV5640 camera ($20)
3. ESP32-S3 with PSRAM ✅ ON HAND
4. **Printed camera gimbal + OV5640/MLX90640 mount** (already printed)
5. H.264 + KLV + TS firmware
6. **Mount gimbal on truck, route cables**

### Phase 5: Ground Station & Oculus (Week 6-8)
1. WiFi 6 router ($100)
2. Meta Quest 3 ($500) or Quest 2 used ($300)
3. MediaMTX + WebXR app development

### Phase 6: Integration & Field Test (Week 8+)
1. **Existing RC truck + CP2 remote** ✅ ON HAND
2. **All printed parts ready** - enclosures, gimbal, mounts, dampers, battery tray, electronics tray
3. Vibration isolation (printed TPU dampers installed between electronics tray and chassis)
4. Full system integration - wire all sensors, MCUs, motor driver
5. Configure RC fallback: CP2 receiver → RP2040 PWM input (4 channels)
6. Field test & calibrate: compass, GPS RTK, thermal thresholds, ToF zones, PID tuning

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