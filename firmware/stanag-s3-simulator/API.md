# STANAG S3 Simulator — API Reference

## Core Types (`stanag_types.h`)

### `jpeg_frame_t`

```c
typedef struct {
    uint8_t *data;      // JPEG compressed data
    size_t  len;        // Data length in bytes
    int     width;      // Image width (1280)
    int     height;     // Image height (720)
    int     index;      // Frame index (0-4)
} jpeg_frame_t;
```

### `yuv420_frame_t`

```c
typedef struct {
    uint8_t  *y;        // Luma plane (width x height)
    uint8_t  *u;        // Cb plane (width/2 x height/2)
    uint8_t  *v;        // Cr plane (width/2 x height/2)
    int      width;
    int      height;
    int      stride_y;
    int      stride_uv;
    uint64_t timestamp_us;
} yuv420_frame_t;
```

### `h264_nalu_t`

```c
typedef struct {
    uint8_t  *data;     // NAL unit bytes (with start code)
    size_t   len;
    int      type;      // NAL type (5=IDR, 7=SPS, 8=PPS, 9=AUD)
    uint64_t timestamp_us;
    bool     keyframe;
} h264_nalu_t;
```

### `klv_packet_t`

```c
typedef struct {
    uint8_t  *data;     // KLV-encoded bytes
    size_t   len;
    uint64_t timestamp_us;
} klv_packet_t;
```

### `ts_packet_t`

```c
typedef struct {
    uint8_t   *data;    // 188-byte TS packets concatenated
    size_t    count;    // Number of packets
    size_t    total_len;// count * 188
    uint64_t  timestamp_us;
} ts_packet_t;
```

### `sensor_data_t`

Aggregates all simulated sensor readings:

```c
typedef struct {
    gps_data_t   gps;   // GPS position, velocity, time
    imu_data_t   imu;   // Accelerometer, gyroscope, quaternion
    mag_data_t   mag;   // Magnetometer heading
    baro_data_t  baro;  // Barometric pressure, altitude, temp
    aht20_data_t aht20; // Temperature, humidity
} sensor_data_t;
```

---

## JPEG Loader (`jpeg_loader.c` / `jpeg_loader.h`)

### `esp_err_t jpeg_loader_init(void)`
Mounts SPIFFS partition `spiffs`, pre-allocates YUV buffer in PSRAM (with internal RAM fallback).
- **Returns:** `ESP_OK` on success, ESP error code on failure

### `jpeg_frame_t* jpeg_loader_get_next(void)`
Returns next JPEG frame from SPIFFS, cycling `frame_1.jpg` → `frame_5.jpg`.
- **Returns:** Pointer to static `jpeg_frame_t`, or `NULL` on failure

### `yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame)`
Converts JPEG to YUV420. On ESP32-S3 v5.5.6 (no HW JPEG decode), returns a dummy test-pattern YUV frame.
- **Returns:** Pointer to static `yuv420_frame_t`, or `NULL` on failure

### `void jpeg_loader_free_yuv(yuv420_frame_t *yuv)`
No-op (static buffer, nothing to free).

### `esp_err_t jpeg_loader_deinit(void)`
Frees frame data buffers, YUV buffer, unmounts SPIFFS.

---

## Sensor Simulator (`sensor_sim.c` / `sensor_sim.h`)

### `esp_err_t sensor_sim_init(void)`
Resets frame counter. Returns `ESP_OK`.

### `void sensor_sim_get_data(sensor_data_t *out, uint32_t frame_num)`
Fills `out` with simulated sensor data for a circular flight path (500 m radius, ~6.6 s period at 30 fps) around Aarhus, Denmark (56.1735°N, 10.2040°E).

| Sensor | Output |
|---|---|
| GPS | lat/lon/alt/heading, RTK fix (fix=4), 12 sats |
| IMU | accel/gyro/quaternion, banked turn ±10° |
| Magnetometer | heading matches GPS, raw XYZ |
| Barometer | pressure/altitude/temp with lapse rate |
| AHT20 | cabin temp 20-24°C, humidity 45-55% |

### `void sensor_sim_update(void)`
No-op (frame-based simulation).

### `esp_err_t sensor_sim_deinit(void)`
Returns `ESP_OK`.

---

## H.264 Encoder (`h264_encoder.c` / `h264_encoder.h`)

Software fallback encoder producing valid H.264 High Profile Level 4.1 bitstream.

### `esp_err_t h264_encoder_init(void)`
Builds SPS, PPS, and AUD NAL units once. Returns `ESP_OK` on success.

### `h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv)`
Encodes one frame. First frame emits SPS + PPS + AUD + IDR; subsequent frames emit AUD + IDR.
- **Returns:** Pointer to static `h264_nalu_t`, or `NULL` on failure

### `esp_err_t h264_encoder_deinit(void)`
Resets encoder state. Returns `ESP_OK`.

**Output NAL units:**
| Type | Value | When |
|---|---|---|
| SPS | 7 | First frame only |
| PPS | 8 | First frame only |
| AUD | 9 | Every frame |
| IDR | 5 | Every frame |

---

## KLV Builder (`klv_builder.c` / `klv_builder.h`)

MISB ST 0601 UAS Local Set KLV encoder with 2-byte keys and 2-byte lengths (big-endian).

### `esp_err_t klv_builder_init(void)`
Returns `ESP_OK`.

### `klv_packet_t* klv_builder_build(sensor_data_t *sensors, uint32_t frame_num)`
Builds KLV packet with the following tags:

| Key | Name | Value | Encoding |
|---|---|---|---|
| 0x0001 | Position | Lat/Lon/Height | deg7 / deg7 / mm (int32) |
| 0x0002 | Velocity NED | vN / vE / vD | mm/s (int32 ×3) |
| 0x0003 | Platform Heading | heading | deg5 (int32) |
| 0x0102 | Barometric Altitude | altitude | mm (int32) |
| 0x0103 | Temperature | temp × 10 | int16 |
| 0x0104 | Magnetic Heading | heading | deg5 (int32) |
| 0xFFFF | Timestamp | tow_ms × 1000 | uint32 µs |

- **Returns:** Allocated `klv_packet_t*`, or `NULL` on failure. Caller must free with `klv_packet_free()`.

### `void klv_packet_free(klv_packet_t *packet)`
Frees `packet->data` and `packet`.

### `esp_err_t klv_builder_deinit(void)`
Returns `ESP_OK`.

---

## MPEG-TS Muxer (`ts_muxer.c` / `ts_muxer.h`)

### `esp_err_t ts_muxer_init(void)`
Initializes continuity counters and static buffer. Returns `ESP_OK`.

### `ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv)`
Muxes H.264 NALU and optional KLV packet into MPEG-TS packets:

| Packet | PID | Content |
|---|---|---|
| PAT | 0x0000 | Program Association Table (every frame) |
| PMT | 0x1000 | Program Map Table (every frame) |
| Video PES | 0x0100 | H.264 NALU with PCR |
| KLV PES | 0x0101 | KLV metadata (private stream) |
| Null | 0x1FFF | Padding |

- **Returns:** Pointer to static `ts_packet_t`, or `NULL` on failure
- **Note:** Static buffer — do not free; `ts_packet_free()` is a no-op

### `void ts_packet_free(ts_packet_t *ts)`
No-op (static buffer).

### `esp_err_t ts_muxer_deinit(void)`
Resets continuity counters. Returns `ESP_OK`.

---

## Network TX (`network_tx.c` / `network_tx.h`)

### `esp_err_t network_tx_init(void)`
Creates non-blocking UDP socket to `192.168.4.2:8888`. ESP-NOW init is deferred (requires WiFi).

### `esp_err_t network_tx_espnow_init(void)`
Initializes ESP-NOW after WiFi is started. Adds broadcast peer `FF:FF:FF:FF:FF:FF`.

### `void network_tx_send(ts_packet_t *ts)`
Sends TS packet via UDP and/or ESP-NOW (whichever is enabled and available).

### `void network_tx_set_udp_dest(const char *ip, uint16_t port)`
Configures UDP destination (default: `192.168.4.2:8888`).

### `void network_tx_set_espnow_peer(const uint8_t *mac)`
Configures ESP-NOW peer MAC address.

### `void network_tx_deinit(void)`
Closes UDP socket, removes ESP-NOW peer, deinitializes ESP-NOW.

---

## UART Telemetry (`uart_telemetry.c` / `uart_telemetry.h`)

### `esp_err_t uart_telemetry_init(void)`
Initializes UART1 (GPIO 17 TX, GPIO 18 RX, 115200 baud, 8N1). Returns `ESP_OK`.

### `void uart_telemetry_send(sensor_data_t *sensors, h264_nalu_t *nalu, uint32_t frame_num)`
Sends JSON telemetry via UART1:

```json
{
  "frame":0,
  "timestamp_us":123456789000,
  "gps":{"lat":56.173500,"lon":10.204000,"alt":50.0,"heading":135.0,"fix":4,"sats":12},
  "imu":{"roll":0.0,"pitch":0.0,"yaw":135.0},
  "mag":{"heading":135.0},
  "baro":{"altitude":50.0,"pressure":1013.1,"temp":20.0},
  "aht20":{"temp":22.0,"humidity":50.0},
  "h264":{"type":5,"keyframe":true,"len":128}
}
```

Falls back to `printf` (stdout) when UART is not initialized.

### `void uart_telemetry_deinit(void)`
Deletes UART driver. Returns `ESP_OK`.

---

## Entry Point (`main.c`)

### `void app_main(void)`
Main entry point. Runs at 30 fps:

1. Disable task watchdog
2. Initialize NVS
3. Initialize network stack (netif + event loop)
4. Initialize subsystems: JPEG loader, sensor sim, H.264 encoder, KLV builder, TS muxer, network TX, UART telemetry
5. Loop forever:
   - Get next JPEG frame
   - Decode to YUV (dummy on S3)
   - Encode to H.264
   - Get sensor data
   - Build KLV metadata
   - Mux into MPEG-TS
   - Send via network TX
   - Send telemetry via UART

WiFi/ESP-NOW initialization is commented out (blocks in QEMU). Uncomment for hardware testing.

---

## Build System

### `CMakeLists.txt` (project root)
- Sets `EXTRA_COMPONENT_DIRS` to `components/` and `test/`
- Includes ESP-IDF `project.cmake`
- Calls `spiffs_create_partition_image(spiffs, data/)` to generate SPIFFS image from JPEG files

### `main/CMakeLists.txt`
Registers component with sources: `main.c`, `jpeg_loader.c`, `sensor_sim.c`, `h264_encoder.c`, `klv_builder.c`, `ts_muxer.c`, `network_tx.c`, `uart_telemetry.c`
Requires: `driver`, `esp_wifi`, `esp_netif`, `esp_event`, `nvs_flash`, `esp_timer`, `freertos`, `esp_psram`, `mbedtls`, `cxx`, `log`, `spiffs`

### `test/CMakeLists.txt`
Registers test component with Unity framework, requires `main` + all IDF dependencies above.

### `data/CMakeLists.txt`
Lists 5 JPEG files for SPIFFS embedding; verifies they exist at configure time.
