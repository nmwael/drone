# STANAG S3 Simulator — Testing Guide

## Test Suite Overview

64 Unity unit tests across 8 modules, registered in `test/test_runner.c`.

```mermaid
flowchart TB
    subgraph TestRunner["test_runner.c<br/>app_main entry"]
        direction TB
        KLV[KLV Builder<br/>8 tests]
        TSM[TS Muxer<br/>11 tests]
        H264[H.264 Encoder<br/>9 tests]
        JL[JPEG Loader<br/>10 tests]
        SS[Sensor Simulator<br/>14 tests]
        NET[Network TX<br/>5 tests]
        UART[UART Telemetry<br/>4 tests]
    end

    subgraph Unity["Unity Framework<br/>(esp-idf component)"]
        U[UNITY_BEGIN()<br/>RUN_TEST()<br/>UNITY_END()]
    end

    TestRunner --> Unity

    classDef mod fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    class KLV,TSM,H264,JL,SS,NET,UART mod
```

## Test Coverage Matrix

| Module | Tests | What's Tested |
|---|---|---|
| KLV Builder | 8 | init, valid/NULL data, key presence, position/velocity/temp encoding, packet free, timestamp |
| TS Muxer | 11 | init, mux valid/NULL, sync byte, PIDs, continuity counter, PUSI, adaptation field, packet size, null packet |
| H.264 Encoder | 9 | init, encode valid/NULL, start codes, IDR type, timestamps, keyframe flag, multiple encodes, NAL type |
| JPEG Loader | 10 | init, get_next, frame cycling, decode, NULL/empty frames, YUV layout, test pattern, free, static buffer |
| Sensor Simulator | 14 | init, GPS/IMU/full data, circular path, altitude, heading, velocity, IMU banked turn, accel Z, gyro Z, mag heading, baro pressure/temp, AHT20 temp/humidity, GPS time/week/sats/PDOP |
| Network TX | 5 | init, set UDP dest, set ESP-NOW peer, send valid/NULL/empty, deinit |
| UART Telemetry | 4 | init, send valid, NULL sensors/NALU, deinit |

## Running Tests

### On QEMU (recommended for devcontainer)

```bash
cd firmware/stanag-s3-simulator

# Build test app
idf.py -DTEST_COMPONENT=1 build

# Run on QEMU (60 second timeout)
timeout 60 idf.py -p /dev/null qemu monitor 2>&1 | tee test_output.log

# Check results
grep "Tests passed" test_output.log
# or
grep "Tests failed" test_output.log
```

### CI Pipeline (Woodpecker / GitHub Actions)

`.woodpecker.yml` defines the CI stages:

```mermaid
flowchart TB
    Build[Build ESP32-S3] --> QEMU[QEMU Test]
    Build --> Unit[Unit Tests (QEMU)]
    Build --> Static[Cppcheck Static Analysis]
    Build --> Size[Size Report]
    QEMU --> Validate[Validate TS Structure]
    Unit --> Pass[Pass/Fail]
    Static --> Pass
    Size --> Pass
    Validate --> Pass
    Build --> Upload[Upload Artifacts]
    Upload --> Release[Create Release on Tag]

    classDef stage fill:#fff3e0,stroke:#e65100,stroke-width:2px
    class Build,QEMU,Unit,Static,Size,Validate,Upload,Release stage
```

### Local Hardware

```bash
idf.py -p /dev/ttyUSB0 flash monitor
# Watch serial output for test results
```

## Example Test Output

```
Starting unit tests...
[PASS] test_klv_builder_init
[PASS] test_klv_builder_build_valid_data
[PASS] test_klv_builder_build_null_sensors
...
[PASS] test_uart_telemetry_deinit
All tests completed
-----------------------
64 Tests 0 Failures 0 Ignored
OK
```

## Adding a New Test

1. Create `test/<module>_test.c` or add to existing file
2. Implement test functions following Unity conventions:

```c
#include "unity.h"
#include "klv_builder.h"
#include "stanag_types.h"
#include "sensor_sim.h"

void setUp(void) {
    klv_builder_init();
    sensor_sim_init();
}

void tearDown(void) {}

void test_my_new_feature(void) {
    sensor_data_t sensors;
    sensor_sim_get_data(&sensors, 0);
    klv_packet_t *pkt = klv_builder_build(&sensors, 0);
    TEST_ASSERT_NOT_NULL(pkt);
    TEST_ASSERT_GREATER_THAN(0, pkt->len);
    klv_packet_free(pkt);
}
```

3. Add `extern` declaration and `RUN_TEST()` call in `test/test_runner.c`

## Test Dependencies

`test/CMakeLists.txt` requires:

```cmake
REQUIRES
    unity
    main
    driver
    esp_wifi
    esp_netif
    esp_event
    nvs_flash
    esp_timer
    freertos
    esp_psram
    mbedtls
    cxx
    log
    spiffs
```

## CI Failure Modes

| Stage | Common Failure | Resolution |
|---|---|---|
| Build | SPIFFS image generation fails | Check `CONFIG_PARTITION_TABLE_CUSTOM=y` |
| Build | Undefined `esp_psram_*` symbols | Check `CONFIG_SPIRAM=y` |
| QEMU Test | WiFi/ESP-NOW blocks | Skipped by design in `main.c` |
| Static Analysis | cppcheck warnings | Review `cppcheck.log` |
| Release | Missing tag | Tag must match `v*` pattern |
