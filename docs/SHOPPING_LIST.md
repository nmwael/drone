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

## Vision Sensors — **Buy from EU/DK Stores**

| Item | Qty | Status | Est. Cost (EUR) | EU/DK Store Links |
|------|-----|--------|-----------------|-------------------|
| **Thermal Camera** | 0 | ❌ **REMOVED** | **€0** | MLX90640 removed from scope — using OV5640 day camera only |
| **IR Camera (Night Vision)** | 1 | 🛒 TO BUY | **€15-25** | **OV5640 5MP Camera Module** (CSI/Parallel, IR filter removed)<br>→ [Arducam EU](https://eu.arducam.com/product/ov5640-5mp-camera-module/) / [BerryBase DE](https://www.berrybase.de/kameras/arducam) / [Exp-Tech DE](https://www.exp-tech.de/kameras/arducam) / [Watterott DE](https://www.watterott.com/de/Kameras/Arducam) / [Amazon.de](https://www.amazon.de/s?k=OV5640+Camera+Modul) |
| **Thermal Camera Alt (lower res)** | - | - | €25-35 | AMG8833 8×8 (Panasonic) - cheaper but low res<br>→ [Mouser DK](https://www.mouser.dk/ProductDetail/Panasonic/AMG8833) / [DigiKey DK](https://www.digikey.dk/en/products/detail/panasonic-electronic-components/AMG8833/5907444) |

---

## Navigation Sensors — **Buy from EU/DK Stores**

| Item | Qty | Status | Est. Cost (EUR) | EU/DK Store Links |
|------|-----|--------|-----------------|-------------------|
| **GNSS/GPS** | 1 | 🛒 TO BUY | **€6.90** | **u-blox NEO-6M GPS TTL receiver incl. antenna** (2-5m accuracy)<br>→ **[BerryBase DE](https://www.berrybase.de/en/detail/019234a35ca671b396889711157d1ce9)** (u-blox NEO-6M module with patch antenna, 5V TTL) / [Mouser DK](https://www.mouser.dk/c/gps-gnss-modules) / [DigiKey DK](https://www.digikey.dk/en/products/filter/gps-gnss-modules/224) / [BerryBase DE](https://www.berrybase.de/sensoren/positionierung/gps-gnss) / [Holybro EU](https://holybro.com/collections/gps/products/m8n-gps-module) / [AliExpress](https://www.aliexpress.com/wholesale?SearchText=NEO-6M+GPS) / [Banggood EU](https://eu.banggood.com/search/gps.html?keyword=NEO-6M) |
| **GPS Antenna** | 1 | ✅ **INCLUDED** | €0 | **Built into NEO-6M module** (patch antenna on module)<br>→ Optional upgrade: u-blox ANN-MB-00 or Taoglas AA.161<br>→ [Mouser DK](https://www.mouser.dk/c/rf-if-and-rfid/gps-antennas) / [DigiKey DK](https://www.digikey.dk/en/products/filter/rf-antennas/232) / [BerryBase DE](https://www.berrybase.de/sensoren/positionierung/gps-antennen) / [Amazon.de](https://www.amazon.de/s?k=GPS+patch+antenna) |
| **Magnetometer** | 1 | 🛒 TO BUY | **€8-15** | **QMC5883L** (I2C, 16-bit)<br>→ [Mouser DK](https://www.mouser.dk/c/sensors/magnetic-sensors) / [DigiKey DK](https://www.digikey.dk/en/products/filter/magnetic-sensors/659) / [BerryBase DE](https://www.berrybase.de/sensoren/magnetfeldsensoren) / [Exp-Tech DE](https://www.exp-tech.de/sensoren/magnetfeldsensoren) / [Amazon.de](https://www.amazon.de/s?k=QMC5883L) |
| **IMU (Accel + Gyro)** | 1 | 🛒 TO BUY | **€12-20** | **ICM-20948** (9-DoF, DMP, I2C/SPI)<br>→ [Mouser DK](https://www.mouser.dk/ProductDetail/TDK-InvenSense/ICM-20948) / [DigiKey DK](https://www.digikey.dk/en/products/detail/tdk-invensense/ICM-20948/8708445) / [RS Components DK](https://dk.rs-online.com/web/p/imu-sensors) / [Watterott DE](https://www.watterott.com/de/Sensoren/IMU) |
| **Barometer / Pressure** | 1 | ✅ ON HAND | €0 | **BMP280** (I2C, 0x76/0x77) |
| **Temperature / Humidity** | 1 | ✅ ON HAND | €0 | **AHT20** (I2C, 0x38) |

---

## Power & Actuation — **Buy from EU/DK Stores**

| Item | Qty | Status | Est. Cost (EUR) | EU/DK Store Links |
|------|-----|--------|-----------------|-------------------|
| **Camera Mount (Fixed)** | 1 | 🖨️ **PRINT** | €0 | Fixed mount for OV5640 day camera (printed bracket) |
| **3.3V Regulator (3A+)** | 1 | ✅ **ON HAND** | €0 | Buck converter (3.3V 3A+) |
| **5V BEC (3A+)** | 1 | ✅ **ON HAND** | €0 | Buck converter (5V 3A+) |
| **Power Distribution Box** | 1 | 🖨️ **PRINT** | **€0 (filament)** | Printed enclosure with fused 5V/3.3V rails, XT60 in/out |
| **Main Battery** | 1 | 🛒 TO BUY | **€30-60** | **2S LiPo 5000-8000mAh 50C+** (7.4V) or **3S 5000mAh** (11.1V)<br>→ [HobbyKing EU](https://hobbyking.com/en_us/lipo-batteries.html) / [Banggood EU](https://eu.banggood.com/Lipo-Batteries-c-100000001.html) / [RC-Tech DK](https://www.rc-tech.dk/lipo-batterier) / [HobbyStore DK](https://www.hobbystore.dk/lipo-batterier) / [Amazon.de](https://www.amazon.de/s?k=2S+LiPo+5000mAh) |
| **Nokia BP-4L Batteries** | Multiple | ✅ **ON HAND** | **€0** | **3.7V ~1500mAh Li-ion** (Nokia phone batteries)<br>→ Use for: **Electronics/sensor power backup**, ESP32/ESP8266 standalone testing<br>→ Can wire 2S (7.4V) or parallel for capacity<br>→ **BMS ✅ ON HAND**, Need: JST connectors, charging solution |
| **libr160 Li-ion Cells** | 2 | ✅ **ON HAND** | **€0** | **3.7V 1050mAh Li-ion** (model: libr160)<br>→ Use for: **Electronics backup**, sensor power, ESP32 testing<br>→ Can wire in parallel (2100mAh @ 3.7V) or 2S series (7.4V)<br>→ **BMS ✅ ON HAND**, Need: JST connectors |
| **Battery Charger** | 1 | 🔄 SALVAGED | €0 | CP2 charger (1S/2S) - may need upgrade for 2S/3S |

> **Note:** Truck is **100% driving functional** — CP2 remote → ESC + steering servo works perfectly. No motor driver, no servo controller, no MCU in the control loop. 3.3V/5V regulators power **MCUs and sensors only**. This project adds sensor suite (vision, GPS, IMU) for autonomy/teleop on top of a working truck.

---

## Communications & Ground Station — **Buy from EU/DK Stores**

| Item | Qty | Status | Est. Cost (EUR) | EU/DK Store Links |
|------|-----|--------|-----------------|-------------------|
| **WiFi 5 Router / AP** | 1 | ✅ **ON HAND** | **€0** | Existing router (5 GHz capable, use for local network) |
| **External Antenna (ESP32-S3)** | 1 | 🛒 TO BUY | **€10-20** | 2.4/5 GHz dual-band PCB antenna or u.FL → SMA pigtail<br>→ [Mouser DK](https://www.mouser.dk/c/rf-antennas) / [DigiKey DK](https://www.digikey.dk/en/products/filter/rf-antennas/232) / [RS Components DK](https://dk.rs-online.com/web/p/rf-antennas) / [Conrad DK](https://www.conrad.dk/p/wlan-antennen) / [Amazon.de](https://www.amazon.de/s?k=u.fl+antenna+2.4ghz+5ghz) |
| **Ground Station PC / Raspberry Pi 5** | 1 | 🛒 TO BUY | **€0 (PC) / €80 (Pi 5)** | PC preferred for WebXR; Pi 5 can run MediaMTX only<br>→ [Raspberry Pi DK](https://www.raspberrypi.dk/) / [Mouser DK](https://www.mouser.dk/c/single-board-computers/raspberry-pi) / [RS Components DK](https://dk.rs-online.com/web/p/raspberry-pi) / [BerryBase DE](https://www.berrybase.de/raspberry-pi/raspberry-pi-5) |
| **Meta Quest 2** | 1 | ✅ **ON HAND** | **€0** | Used Quest 2 (€300 value) — WebXR capable |

---

## Mechanical / Chassis — **Using Existing RC Monster Truck + CP2 Remote**

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| **RC Monster Truck** | 1 | ✅ **ON HAND** | **$0** | **100% functional** — drives perfectly with CP2 remote, no mods needed |
| **CP2 2.4GHz Remote** | 1 | ✅ **ON HAND** | **$0** | **Direct to ESC + steering servo** — primary control, always works |
| **Camera Mount (Fixed)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printed bracket for OV5640 day camera (mounts to Universal Box front) |
| **Sensor Enclosures** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - C3 hub (GPS/IMU/Mag/Baro/AHT20) inside Universal Box |
| **VL53L5CX Mounts** | 4 | 🖨️ **PRINT** | **$0 (filament)** | Printable - front, rear, left, right brackets (truck-specific) |
| **Vibration Dampers** | 4-8 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - TPU Shore 60-80A for IMU/camera isolation |
| **GPS Antenna Mast** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - elevates ZED-F9P antenna above truck body |
| **Antenna Mount (ESP32-S3)** | 1 | 🖨️ **PRINT** | **$0 (filament)** | Printable - u.FL/SMA external antenna bracket |
| **Wire Routing Clips** | 10+ | 🖨️ **PRINT** | **$0 (filament)** | Printable - cable management along chassis |
| **Battery Tray / Strap** | 1 | 🖨️ **PRINT (TPU)** | **$0 (filament)** | Printable - secure 2S/3S LiPo to existing chassis |

| **Universal Electronics Box** | 1 | 🖨️ **PRINT** | **€0 (filament)** | **Single modular enclosure** — contains ESP32-S3, ESP32-C3, 3×RP2040, ESP8266, power distro (buck converters), BMS, sensor hub (GPS/IMU/Mag/Baro/AHT20), all wiring. Mounts on ANY vehicle via universal bolt pattern (M6/M8 slots). Replaces: separate electronics tray, C3 hub enclosure, power distro box. |
| **Sensor Enclosures** | 0 | ❌ **REPLACED** | **€0** | Consolidated into Universal Electronics Box |
| **Power Distribution Box** | 0 | ❌ **REPLACED** | **€0** | Consolidated into Universal Electronics Box |
| **Electronics Tray** | 0 | ❌ **REPLACED** | **€0** | Consolidated into Universal Electronics Box |
| **Battery Tray / Strap** | 1 | 🖨️ **PRINT (TPU)** | **€0 (filament)** | Printable - secure 2S/3S LiPo to existing chassis (separate from electronics box) |
| **VL53L5CX Mounts** | 0 | ❌ **NOT NEEDED** | **€0** | ToF array removed from project |
| **Vibration Dampers** | 4-8 | 🖨️ **PRINT (TPU)** | **€0 (filament)** | Printable - TPU Shore 60-80A for IMU/camera isolation (between Universal Box and vehicle) |
| **GPS Antenna Mast** | 1 | 🖨️ **PRINT** | **€0 (filament)** | Printable - elevates ZED-F9P antenna above vehicle body (mounts to Universal Box) |
| **Antenna Mount (ESP32-S3)** | 1 | 🖨️ **PRINT** | **€0 (filament)** | Printable - u.FL/SMA external antenna bracket (on Universal Box) |
| **Wire Routing Clips** | 10+ | 🖨️ **PRINT** | **€0 (filament)** | Printable - cable management inside Universal Box |
| **Servo Adapters / Horns** | 2-4 | 🖨️ **PRINT** | **€0 (filament)** | Printable - adapt CP2 servos to gimbal |
| Wiring / Connectors | 1 | 🛒 TO BUY | **€20-40** | JST-SH, JST-GH, DuPont, XT60, heat shrink, cable sleeving (internal wiring for Universal Box)<br>→ [Mouser DK](https://www.mouser.dk/c/connectors/wire-to-board-connectors) / [DigiKey DK](https://www.digikey.dk/en/products/filter/connectors-interconnects/211) / [RS Components DK](https://dk.rs-online.com/web/p/connectors) / [Conrad DK](https://www.conrad.dk/p/steckverbinder) / [BerryBase DE](https://www.berrybase.de/steckverbinder-kabel) / [Amazon.de](https://www.amazon.de/s?k=JST+SH+Stecker+XT60) |
| Heat-set Inserts (M3/M4) | 50+ | ✅ **ON HAND** | €0 | For strong bolt-on printed parts |
| M3/M4 Hardware (screws, nuts, standoffs) | 1 kit | 🛒 TO BUY | **€15-25** | Brass standoffs, nylon lock nuts, button-head screws<br>→ [Mouser DK](https://www.mouser.dk/c/hardware-fasteners-accessories) / [RS Components DK](https://dk.rs-online.com/web/p/fasteners-fixings) / [Conrad DK](https://www.conrad.dk/p/schrauben-muttern) / [BerryBase DE](https://www.berrybase.de/mechanik/befestigungstechnik) / [Watterott DE](https://www.watterott.com/de/Mechanik/Befestigung) / [Amazon.de](https://www.amazon.de/s?k=M3+Messing+H%C3%BClse+Gewindeeinsatz) |

> **Design Approach:** Measure your truck's chassis hole patterns, body mount points, and servo spline type. Design printed parts in Fusion 360 / FreeCAD to bolt directly to existing mounting points. Use **TPU dampers** between electronics tray and chassis for vibration isolation.

---

## 3D Printing Filament (FDM Printer ✅ ON HAND) — **Buy from EU/DK Stores**

| Material | Qty | Status | Use Case | EU/DK Store Links |
|----------|-----|--------|----------|-------------------|
| **PETG** | On hand | ✅ **ON HAND** | Functional parts: enclosures, mounts, brackets (UV/heat resistant) | |
| **TPU** | On hand | ✅ **ON HAND** | Vibration dampers, battery straps, flexible mounts (Shore 60-80A) | |
| **PLA** | On hand | ✅ **ON HAND** | Prototypes, wire clips, non-structural test prints | |
| **ASA** | 1 roll | 🛒 **OPTIONAL** | Outdoor/UV parts: GPS mast, antenna mounts (€30-40 if needed) | [BerryBase DE](https://www.berrybase.de/3d-druck/filament/asa-filament) / [Watterott DE](https://www.watterott.com/de/3D-Druck/Filament/ASA) / [Exp-Tech DE](https://www.exp-tech.de/3d-druck/filament/asa) / [Filamentz DE](https://www.filamentz.de/asa-filament) / [Amazon.de](https://www.amazon.de/s?k=ASA+Filament+1.75mm) |

> **Print Settings for Functional Parts:** 0.2mm layer, 3-4 walls, 25-30% gyroid infill, 220-240°C nozzle (PETG), 70-80°C bed. Dry filament 4h @ 65°C before printing PETG/TPU.

---

## Development Tools

| Item | Qty | Status | Est. Cost | Notes |
|------|-----|--------|-----------|-------|
| **USB-UART Adapter** | 0 | ❌ **NOT NEEDED** | $0 | All boards have onboard USB (ESP32-S3, C3, RP2040, ESP8266) |
| **JTAG Debugger (ESP-Prog / J-Link)** | 0 | ❌ **NOT NEEDED** | $0 | ESP32 has onboard USB JTAG; printf debugging sufficient |
| **Logic Analyzer (Saleae / DSLogic)** | 0 | ❌ **NOT NEEDED** | $0 | I2C/SPI/UART debugging via ESP32 built-in tools + printf |
| Multimeter / Oscilloscope | 1 | ✅ ON HAND | $0 | Basic bench tools |

---

## Cost Summary (EUR)

| Category | Est. Cost (Min) | Est. Cost (Max) |
|----------|----------------|----------------|
| Vision Sensors (OV5640) | €15 | €25 |
| **Navigation Sensors (GPS + IMU + Mag)** | **€30** | **€55** |
| Power & Actuation (battery, regs, box) | €30 | €60 |
| Comms & Ground Station (router, Quest) | €10 | €100 |
| Mechanical (Printed Box + Gimbal) | €5 | €15 |
| Dev Tools | €0 | €0 |
| **ASA Filament (optional)** | **€0** | **€40** |
| **TOTAL** | **~€85** | **~€310** |

> **Notes:**
> - **Min cost** assumes: PC ground station, AliExpress/BerryBase/Exp-Tech for sensors
> - **Max cost** assumes: PC ground station, Mouser/DigiKey/RS Components for all parts
> - **Already owned saves ~€750-850**: MCUs, BMP280, AHT20, CP2 servos/motors/charger, **RC truck + remote**, basic tools, **FDM printer + PETG/TPU/PLA filament**, **Quest 2 (€300)**, **WiFi 5 router**, **heat-set inserts (M3/M4)**, **buck converters (3.3V/5V)**
> - **3D Printing saves ~€150-400** vs buying enclosures, gimbal, mounts, dampers, battery trays (only hardware to buy)
> - **Cost Savings Applied:**
>   - ❌ MLX90640 thermal camera removed (-€50-70)
>   - ❌ RP2040 ToF arrays removed (-€216-300)
>   - ❌ USB-UART adapter removed (onboard USB)
>   - ❌ JTAG debugger removed (onboard USB JTAG)
>   - ❌ Logic Analyzer removed (built-in tools)
>   - ❌ **ZED-F9P RTK GPS replaced with NEO-6M (-€173-243)**
>   - ✅ OV5640 only (day camera, €15-25)
>   - ✅ All enclosures 3D printed (PETG/TPU on hand)
>   - ✅ RC truck + remote already owned
>   - ✅ Quest 2 + WiFi 5 router already owned
>   - ✅ Buck converters + BMS + heat-set inserts on hand
>   - ❌ USB-UART adapter removed (onboard USB)
>   - ❌ JTAG debugger removed (onboard USB JTAG)
>   - ❌ Logic Analyzer removed (built-in tools)
>   - ✅ OV5640 only (day camera, €15-25)
>   - ✅ All enclosures 3D printed (PETG/TPU on hand)
>   - ✅ RC truck + remote already owned
>   - ✅ Quest 2 + WiFi 5 router already owned
>   - ✅ Buck converters + BMS + heat-set inserts on hand

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
1. u-blox NEO-6M GPS + antenna (€6.90)
2. QMC5883L (€8-15)
3. ICM-20948 (€12-20)
4. BMP280 + AHT20 ✅ ON HAND
5. ESP32-C3 firmware: GPS + Mag + IMU + EKF
6. **Install sensors in printed C3 hub enclosure**

### Phase 2: Vision & STANAG (Week 2-4)
1. OV5640 day camera (€15-25)
2. ESP32-S3 with PSRAM ✅ ON HAND
3. **Printed camera gimbal + OV5640 mount** (already printed)
4. H.264 + KLV + TS firmware
5. **Mount gimbal on truck, route cables**

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

## Supplier Quick Links — **EU/DK Focus**

| Supplier | Website | Region | Notes |
|----------|---------|--------|-------|
| **Mouser DK** | [mouser.dk](https://www.mouser.dk) | DK/EU | Fast shipping, huge stock, genuine parts, DKK pricing |
| **DigiKey DK** | [digikey.dk](https://www.digikey.dk) | DK/EU | Huge selection, fast shipping, genuine parts |
| **RS Components DK** | [dk.rs-online.com](https://dk.rs-online.com) | DK/EU | Good stock, next-day delivery, trade account discounts |
| **Conrad DK** | [conrad.dk](https://www.conrad.dk) | DK/EU | Local Danish store, good for hardware/connectors |
| **BerryBase DE** | [berrybase.de](https://www.berrybase.de) | DE/EU | Maker-focused, good for Pi/Arduino/sensors, ships to DK |
| **Exp-Tech DE** | [exp-tech.de](https://www.exp-tech.de) | DE/EU | Maker electronics, sensors, displays, ships to DK |
| **Watterott DE** | [watterott.com](https://www.watterott.com) | DE/EU | Robotics/maker focus, good for sensors/IMUs |
| **Filamentz DE** | [filamentz.de](https://www.filamentz.de) | DE/EU | Quality filament (PETG/TPU/ASA), ships to DK |
| **HobbyKing EU** | [hobbyking.com](https://hobbyking.com/en_us/warehouse/eu.html) | EU | LiPo batteries, RC gear, EU warehouse |
| **Banggood EU** | [banggood.com](https://eu.banggood.com) | EU | Cheap modules, batteries, EU warehouse option |
| **RC-Tech DK** | [rc-tech.dk](https://www.rc-tech.dk) | DK | Danish RC store, LiPo, chargers, parts |
| **HobbyStore DK** | [hobbystore.dk](https://www.hobbystore.dk) | DK | Danish hobby store, RC, batteries, tools |
| **Amazon.de** | [amazon.de](https://www.amazon.de) | DE/EU | Fast Prime shipping, wide selection |

---

## Part Numbers for Quick Search (EU/DK Stores)

```
OV5640-CAM                 (5MP Day Camera)
NEO-6M-GPS                 (u-blox NEO-6M GPS TTL + antenna) — [BerryBase](https://www.berrybase.de/en/detail/019234a35ca671b396889711157d1ce9)
ANN-MB-00                  (GPS Patch Antenna - optional upgrade)
QMC5883L                   (Magnetometer)
ICM-20948                  (9-DoF IMU)
BMP280                     (Pressure - HAVE)
AHT20                      (Temp/Humidity - HAVE)
TPS5430                    (3A Buck Reg 3.3V - HAVE)
LM2596S-5.0                (5V Buck Reg - HAVE)
TCA9548A                   (I2C Mux - if adding ToF later)
```

> **Search tips for DK/EU stores:**
> - Mouser DK: Search by manufacturer part number (e.g., "NEO-6M-GPS")
> - DigiKey DK: Use "Product Index" → search by keyword
> - RS Components DK: Search by "Order Code" or keyword
> - Conrad DK: Search by "Artikelnummer" or keyword
> - For LiPo: Search "2S LiPo 5000mAh 50C" on HobbyKing EU / RC-Tech DK

---

**Next Action:** Order Phase 1 parts (GPS, Mag, IMU) to start ESP32-C3 sensor hub firmware development.