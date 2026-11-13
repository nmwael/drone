# STANAG S3 Simulator — Build Guide

## Prerequisites

- ESP-IDF v5.5.6 (installed via EIM in the devcontainer)
- Target: ESP32-S3 (requires PSRAM, 8 MB+)
- CMake 3.16+, Ninja

## Environment Setup (Devcontainer)

The devcontainer (`post-create.sh`) installs ESP-IDF v5.5.6 via EIM and sources it automatically:

```bash
# In the devcontainer terminal
eim shell v5.5.6
# or simply:
idf.py --version   # should report ESP-IDF v5.5.6
```

## Build Steps

```bash
cd /workspace/firmware/stanag-s3-simulator

# 1. Set target
idf.py set-target esp32s3

# 2. Build
idf.py build

# 3. Flash (hardware only)
idf.py -p /dev/ttyUSB0 flash

# 4. Monitor (hardware only)
idf.py -p /dev/ttyUSB0 monitor
```

## Build Artifacts

| Artifact | Path | Flash Offset | Purpose |
|---|---|---|---|
| Bootloader | `build/bootloader/bootloader.bin` | 0x0 | Second-stage bootloader |
| Partition table | `build/partition_table/partition-table.bin` | 0x8000 | Custom partition table |
| App binary | `build/stanag-s3-simulator.bin` | 0x10000 | Main application |
| SPIFFS image | `build/spiffs.bin` | 0x110000 | 5 JPEG frames |
| ELF | `build/stanag-s3-simulator.elf` | — | Debug symbols |
| Map | `build/stanag-s3-simulator.map` | — | Linker map |

## Configuration

`sdkconfig.defaults` sets two critical options:

```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_SPIRAM=y
```

- **`CONFIG_PARTITION_TABLE_CUSTOM=y`**: Uses `partitions.csv` instead of the built-in `partitions_singleapp.csv`. Without this, the `spiffs` partition is missing and SPIFFS image generation fails.
- **`CONFIG_SPIRAM=y`**: Enables PSRAM support. Required because `jpeg_loader.c` and `main.c` call `esp_psram_get_size()` / `esp_psram_is_initialized()`, and PSRAM is needed for the 1.38 MB YUV frame buffer.

## Size Report

```bash
idf.py size          # Component size breakdown
idf.py size-components  # Per-component sizes
idf.py size-files     # Per-file sizes
```

Typical app binary size: ~390 KB (63% of 1 MB factory partition).

## Clean Build

```bash
idf.py fullclean
idf.py build
```

Or clean everything including build directories:

```bash
cmake --build build --target clean
rm -rf build
```

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `Failed to create SPIFFS image for partition 'spiffs'` | `CONFIG_PARTITION_TABLE_CUSTOM=y` not set | Add to `sdkconfig.defaults` |
| `undefined reference to esp_psram_get_size` | `CONFIG_SPIRAM=y` not set | Add to `sdkconfig.defaults` |
| `idf.py: command not found` | ESP-IDF not sourced | Run `eim shell v5.5.6` first |
