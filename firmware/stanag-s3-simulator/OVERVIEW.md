# STANAG S3 Simulator — Architecture Overview

ESP32-S3 firmware proof-of-concept that loops 5 JPEG frames from SPIFFS, simulates GPS/IMU/Magnetometer/Barometer/AHT20 sensor data, encodes to H.264 (software fallback), builds MISB ST 0601 KLV metadata, muxes into MPEG-TS, and streams via ESP-NOW/UDP for STANAG 4609 compliance testing.

## System Diagram

```mermaid
flowchart TB
    %% Data sources
    SPIFFS[("SPIFFS<br/>frame_1..5.jpg")]
    SensorSim[("Sensor<br/>Simulator")]

    %% Processing pipeline
    JL[JPEG Loader<br/>SPIFFS → YUV420]
    H264[H.264 Encoder<br/>YUV420 → NAL Units]
    KLV[KLV Builder<br/>Sensors → MISB ST 0601]
    TSM[MPEG-TS Muxer<br/>PAT/PMT/PES]

    %% Outputs
    NET[Network TX<br/>ESP-NOW / UDP]
    UART[UART Telemetry<br/>JSON @ 115200]

    %% Main loop
    Main[app_main<br/>30 fps loop]

    SPIFFS --> JL
    SensorSim --> KLV
    JL --> H264
    H264 --> TSM
    KLV --> TSM
    TSM --> NET
    TSM --> UART
    Main --> JL
    Main --> SensorSim
    Main --> H264
    Main --> KLV
    Main --> TSM
    Main --> NET
    Main --> UART

    classDef data fill:#e1f5fe,stroke:#01579b,stroke-width:2px
    classDef proc fill:#fff3e0,stroke:#e65100,stroke-width:2px
    classDef out fill:#e8f5e9,stroke:#1b5e20,stroke-width:2px
    class SPIFFS,SensorSim data
    class JL,H264,KLV,TSM proc
    class NET,UART out
```

## Component Diagram

```mermaid
flowchart LR
    subgraph Main["main/"]
        direction TB
        main[main.c]
        jl[jpeg_loader.c/.h]
        ss[sensor_sim.c/.h]
        h264[h264_encoder.c/.h]
        klv[klv_builder.c/.h]
        tsm[ts_muxer.c/.h]
        net[network_tx.c/.h]
        uart[uart_telemetry.c/.h]
    end

    subgraph Types["stanag_types.h"]
        types[jpeg_frame_t<br/>yuv420_frame_t<br/>h264_nalu_t<br/>klv_packet_t<br/>ts_packet_t<br/>sensor_data_t]
    end

    subgraph Test["test/"]
        direction TB
        tr[test_runner.c<br/>64 Unity tests]
    end

    subgraph Data["data/"]
        imgs[frame_1.jpg … frame_5.jpg]
    end

    subgraph Scripts["scripts/"]
        ke[klv_extractor.sh]
        pt[parse_ts_output.py]
    end

    Main --- Types
    Main --- Data
    Main --- Scripts
    Test --- Main

    classDef mod fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    classDef type fill:#e0f2f1,stroke:#004d40,stroke-width:2px
    classDef test fill:#ffebee,stroke:#b71c1c,stroke-width:2px
    class Main mod
    class Types type
    class Test test
```

## Build Output

```mermaid
flowchart TB
    SDK[ESP-IDF v5.5.6<br/>xtensa-esp32s3-elf-gcc]
    CMake[CMakeLists.txt<br/>+ idf_component_register]
    Part[partitions.csv<br/>custom partition table]
    SDKConfig[sdkconfig.defaults<br/>SPIRAM + custom partition]

    CMake --> Build[(build/)]
    SDK --> Build
    Part --> Build
    SDKConfig --> Build

    Build --> Boot[bootloader/bootloader.bin]
    Build --> PartBin[partition_table/partition-table.bin]
    Build --> App[stanag-s3-simulator.bin]
    Build --> Elf[stanag-s3-simulator.elf]
    Build --> Map[stanag-s3-simulator.map]
    Build --> Spiffs[spiffs.bin]

    classDef tool fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    classDef out fill:#f1f8e9,stroke:#33691e,stroke-width:2px
    class SDK,CMake,Part,SDKConfig tool
    class Boot,PartBin,App,Elf,Map,Spiffs out
```

## Release Artifacts

| Artifact | Path | Purpose |
|---|---|---|
| `stanag-s3-simulator.bin` | `build/stanag-s3-simulator.bin` | Main application binary (flashed at 0x10000) |
| `bootloader.bin` | `build/bootloader/bootloader.bin` | Bootloader (flashed at 0x0) |
| `partition-table.bin` | `build/partition_table/partition-table.bin` | Partition table (flashed at 0x8000) |
| `spiffs.bin` | `build/spiffs.bin` | SPIFFS image with 5 JPEG frames (flashed at 0x110000) |
| `stanag-s3-simulator.elf` | `build/stanag-s3-simulator.elf` | ELF with symbols for debugging |
| `stanag-s3-simulator.map` | `build/stanag-s3-simulator.map` | Linker map file |

## Partition Table

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x100000,
spiffs,   data, spiffs,  0x110000, 0xEF000,
```

| Partition | Offset | Size | Purpose |
|---|---|---|---|
| `nvs` | 0x9000 | 24 KB | Non-volatile storage |
| `phy_init` | 0xF000 | 4 KB | PHY calibration data |
| `factory` | 0x10000 | 1 MB | Main application |
| `spiffs` | 0x110000 | ~960 KB | JPEG frames storage |

## Key Configuration

`sdkconfig.defaults`:

```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y   # Use partitions.csv instead of built-in
CONFIG_SPIRAM=y                    # Enable PSRAM for YUV frame buffers
```

## Quick Start

```bash
# In the container (ESP-IDF v5.5.6 available via EIM)
eim shell v5.5.6

cd firmware/stanag-s3-simulator
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## Related Documentation

- `README.md` — Quick start and feature list
- `MEDIAMTX.md` — Ground station setup (MediaMTX + WebRTC + Meta Quest)
- `mediamtx.yml` — MediaMTX configuration file
- `data/CMakeLists.txt` — SPIFFS image generation
- `test/CMakeLists.txt` — Unity test configuration
- `.woodpecker.yml` — Codeberg CI pipeline
