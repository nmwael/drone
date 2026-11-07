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

## Power & Actuation

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **Servo (Camera Gimbal)** | 1 | 🔄 SALVAGED | $0 | 2nd CP2 servo (pan/tilt for gimbal) |
| **3.3V Regulator (3A+)** | 1 | 🛒 TO BUY | $8-15 | **TPS5430** (3A buck) or **LM2596S-3.3** with heatsink |
| **5V BEC (3A+)** | 1 | 🛒 TO BUY | $8-15 | **LM2596S-5.0** or UBEC 5V/3A |
| **Power Distribution Box** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printed enclosure with fused 5V/3.3V rails, XT60 in/out |
| **Main Battery** | 1 | 🛒 TO BUY | $30-60 | **2S LiPo 5000-8000mAh 50C+** (7.4V) or **3S 5000mAh** (11.1V)<br>→ [Gens Ace](https://www.gensace.de/) / [HRB](https://www.hrb-battery.com/) |
| **Battery Charger** | 1 | 🔄 SALVAGED | $0 | CP2 charger (1S/2S) - may need upgrade for 2S/3S |

> **Note:** Truck is **100% driving functional** — CP2 remote → ESC + steering servo works perfectly. No motor driver, no servo controller, no MCU in the control loop. 3.3V/5V regulators power **MCUs and sensors only**. This project adds sensor suite (vision, thermal, GPS, IMU, ToF) for autonomy/teleop on top of a working truck.

---

## Communications & Ground Station

| Item | Qty | Status | Est. Cost | Recommended Part / Links |
|------|-----|--------|-----------|--------------------------|
| **WiFi 5 Router / AP** | 1 | ✅ **ON HAND** | **$0** | Existing router (5 GHz capable, use for local network) |
| **External Antenna (ESP32-S3)** | 1 | 🛒 TO BUY | $10-20 | 2.4/5 GHz dual-band PCB antenna or u.FL → SMA pigtail |
| **Ground Station PC / Raspberry Pi 5** | 1 | 🛒 TO BUY | **$0 (PC) / $80 (Pi 5)** | PC preferred for WebXR; Pi 5 can run MediaMTX only |
| **Meta Quest 2** | 1 | ✅ **ON HAND** | **$0** | Used Quest 2 ($300 value) — WebXR capable |

---

## Mechanical / Chassis — **Using Existing RC Monster Truck + CP2 Remote**

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| **RC Monster Truck** | 1 | ✅ **ON HAND** | **$0** | **100% functional** — drives perfectly with CP2 remote, no mods needed |
| **CP2 2.4GHz Remote** | 1 | ✅ **ON HAND** | **$0** | **Direct to ESC + steering servo** — primary control, always works |
| **Camera Gimbal (2-axis)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - uses CP2 servos (pan/tilt for OV5640 + MLX90640) |
| **Sensor Enclosures** | 3-4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - C3 hub, motor driver, power distro, battery tray |
| **VL53L5CX Mounts** | 4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - front, rear, left, right brackets (truck-specific) |
| **Vibration Dampers** | 4-8 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - TPU Shore 60-80A for IMU/camera isolation |
| **GPS Antenna Mast** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - elevates ZED-F9P antenna above truck body |
| **Antenna Mount (ESP32-S3)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - u.FL/SMA external antenna bracket |
| **Wire Routing Clips** | 10+ | 🖨️ **PRINT** | **$0 (filament)** | Printable - cable management along chassis |
| **Servo Adapters / Horns** | 2-4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - adapt CP2 servos to steering/gimbal |
| **Battery Tray / Strap** | 1 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - secure 2S/3S LiPo to existing chassis |

| **Electronics Tray** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - mount ESP32-S3, C3, RP2040s to chassis |
| Wiring / Connectors | 1 | 🛒 TO BUY | $20-40 | JST-SH, JST-GH, DuPont, XT60, heat shrink, cable sleeving |
| Heat-set Inserts (M3/M4) | 50+ | ✅ **ON HAND** | $0 | For strong bolt-on printed parts |
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
| **Power & Actuation** | **$16** | **$30** |
| **Comms & Ground Station** | **$10** | **$100** |
| **Mechanical (Printed Fittings)** | **$5** | **$15** |
| Dev Tools | **$75** | **$210** |
| **ASA Filament (optional)** | **$0** | **$40** |
| **TOTAL** | **~$366** | **~$790** |

> **Notes:**
> - **Min cost** assumes: PC ground station, AliExpress/Direct from China for sensors
> - **Max cost** assumes: PC ground station, DigiKey/Mouser for all parts
> - **Already owned saves ~$800-900**: MCUs, BMP280, AHT20, CP2 servos/motors/charger, **RC truck + remote**, basic tools, **FDM printer + PETG/TPU/PLA filament**, **Quest 2 ($300)**, **WiFi 5 router**, **heat-set inserts (M3/M4)**
> - **3D Printing saves ~$150-400** vs buying enclosures, gimbal, mounts, dampers, battery trays (only hardware to buy)

---

## Priority Order (Build Phases)

### Phase 0: 3D Print Preparation (Week 0) 🖨️ **DO FIRST**
1. **Slice & print test coupons** - calibrate flow, temperature, bed adhesion
2. **Print vibration dampers (TPU)** - 4-8 pcs, 30min each, test Shore hardness
3. **Print sensor enclosures (PETG)** - C3 hub, power distro box, electronics tray (4-6h each)
4. **Print camera gimbal (PETG + TPU)** - 6-8h, uses CP2 servos
5. **Print GPS antenna mast, antenna mount, wire clips** - 2-3h total
6. **Install heat-set inserts (M3/M4)** - already on hand, install in printed parts

> **Why Phase 0?** Parts take 15-20h total print time. Start printing while waiting for Phase 1 sensors to arrive.

### Phase 1: Core Navigation (Week 1-2) 🚨 **START HERE**
1. u-blox ZED-F9P + antenna ($180)
2. QMC5883L ($10)
3. ICM-20948 ($15)
4. BMP280 + AHT20 ✅ ON HAND
5. ESP32-C3 firmware: GPS + Mag + IMU + EKF
6. **Install sensors in printed C3 hub enclosure**

### Phase 2: Vision & STANAG (Week 2-4)
1. MLX90640 thermal ($55)
2. OV5640 camera ($20)
3. ESP32-S3 with PSRAM ✅ ON HAND
4. **Printed camera gimbal + OV5640/MLX90640 mount** (already printed)
5. H.264 + KLV + TS firmware
6. **Mount gimbal on truck, route cables**

### Phase 3: Ground Station & Oculus (Week 4-6)
1. WiFi 5 router ✅ ON HAND
2. Meta Quest 2 ✅ ON HAND
3. MediaMTX + WebXR app development

### Phase 4: Integration & Field Test (Week 6+)
1. **Existing RC truck + CP2 remote** ✅ ON HAND
2. **All printed parts ready** - enclosures, gimbal, mounts, dampers, battery tray, electronics tray
3. Vibration isolation (printed TPU dampers installed between electronics tray and chassis)
4. Full system integration - wire all sensors, MCUs
5. Configure RC manual: CP2 receiver → ESC + steering servo (direct, no MCU)
6. Field test & calibrate: compass, GPS RTK, thermal thresholds

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