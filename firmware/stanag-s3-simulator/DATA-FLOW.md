# STANAG S3 Simulator — Data Flow

Per-frame data flow through the 30 fps processing pipeline in `app_main()`.

## Pipeline Sequence

```mermaid
sequenceDiagram
    participant Main as app_main (30 fps)
    participant JL as JPEG Loader
    participant H264 as H.264 Encoder
    participant SS as Sensor Simulator
    participant KLV as KLV Builder
    participant TSM as TS Muxer
    participant NET as Network TX
    participant UART as UART Telemetry

    Main->>+JL: jpeg_loader_get_next()
    JL-->>-Main: jpeg_frame_t (frame_N.jpg)

    Main->>+H264: h264_encoder_encode()
    H264->>+JL: jpeg_loader_decode() → dummy YUV
    JL-->>-H264: yuv420_frame_t (1280x720)
    H264-->>-Main: h264_nalu_t (SPS+PPS+IDR)

    Main->>+SS: sensor_sim_get_data(frame_num)
    SS-->>-Main: sensor_data_t (GPS/IMU/Mag/Baro/AHT20)

    Main->>+KLV: klv_builder_build(&sensors, frame_num)
    KLV-->>-Main: klv_packet_t (MISB ST 0601 KLV)

    Main->>+TSM: ts_muxer_mux(nalu, klv)
    TSM-->>-Main: ts_packet_t (MPEG-TS, 188-byte packets)

    Main->>+NET: network_tx_send(ts)
    NET-->>-Main: (UDP / ESP-NOW broadcast)

    Main->>+UART: uart_telemetry_send(sensors, nalu, frame_num)
    UART-->>-Main: (JSON @ 115200 baud)
```

## Frame Timing Budget (33 ms @ 30 fps)

```mermaid
gantt
    title Per-Frame Budget (33 ms)
    dateFormat  ms
    axisFormat %Lms
    todayMarker off

    section Pipeline
    JPEG load & decode     : 0, 5ms
    H.264 encode           : 5, 15ms
    Sensor simulation      : 20, 2ms
    KLV metadata build     : 22, 3ms
    MPEG-TS mux            : 25, 4ms
    Network TX             : 29, 3ms
    UART telemetry         : 32, 1ms
```

## Data Structures

```mermaid
classDiagram
    class jpeg_frame_t {
        +uint8_t* data
        +size_t len
        +int width
        +int height
        +int index
    }

    class yuv420_frame_t {
        +uint8_t* y
        +uint8_t* u
        +uint8_t* v
        +int width
        +int height
        +uint64_t timestamp_us
    }

    class h264_nalu_t {
        +uint8_t* data
        +size_t len
        +int type
        +uint64_t timestamp_us
        +bool keyframe
    }

    class klv_packet_t {
        +uint8_t* data
        +size_t len
        +uint64_t timestamp_us
    }

    class ts_packet_t {
        +uint8_t* data
        +size_t count
        +size_t total_len
        +uint64_t timestamp_us
    }

    class sensor_data_t {
        +gps_data_t gps
        +imu_data_t imu
        +mag_data_t mag
        +baro_data_t baro
        +aht20_data_t aht20
    }

    jpeg_frame_t --> yuv420_frame_t : decode()
    sensor_data_t --> klv_packet_t : build()
    h264_nalu_t + klv_packet_t --> ts_packet_t : mux()
```

## KLV Metadata Tags (MISB ST 0601)

```mermaid
flowchart TB
    KLV[KLV Packet<br/>per frame] --> P1[0x0001 Position<br/>Lat/Lon/Height<br/>deg7/mm]
    KLV --> P2[0x0002 Velocity NED<br/>mm/s]
    KLV --> P3[0x0003 Platform Heading<br/>deg5]
    KLV --> P4[0x0102 Barometric Altitude<br/>mm]
    KLV --> P5[0x0103 Temperature<br/>int16 × 0.1°C]
    KLV --> P6[0x0104 Magnetic Heading<br/>deg5]
    KLV --> P7[0xFFFF Timestamp<br/>µs]

    classDef kv fill:#fff9c4,stroke:#f57f17,stroke-width:2px
    class P1,P2,P3,P4,P5,P6,P7 kv
```

## MPEG-TS Packet Structure

```mermaid
flowchart TB
    TS[MPEG-TS Stream] --> PAT[PAT<br/>PID 0x0000]
    TS --> PMT[PMT<br/>PID 0x1000]
    TS --> VID[Video PES<br/>PID 0x0100<br/>H.264 + PCR]
    TS --> KLV[KLV PES<br/>PID 0x0101<br/>Private stream]
    TS --> NULL[Null Packets<br/>PID 0x1FFF]

    PAT --> PMT
    PMT --> VID
    PMT --> KLV

    classDef stream fill:#e8f5e9,stroke:#1b5e20,stroke-width:2px
    class PAT,PMT,VID,KLV,NULL stream
```

## Output Destinations

```mermaid
flowchart LR
    TSM[TS Muxer<br/>ts_packet_t]

    TSM --> NET[Network TX<br/>UDP → 192.168.4.2:8888<br/>MediaMTX ingest]
    TSM --> ESP[ESP-NOW<br/>Broadcast peer]
    TSM --> UART[UART1<br/>GPIO 17/18 @ 115200<br/>JSON telemetry]

    NET --> MTX[MediaMTX<br/>WebRTC output<br/>port 8889]
    ESP --> GT[Ground Station<br/>ESP-NOW receiver]
    UART --> C3[ESP32-C3<br/>Sensor Hub]

    classDef out fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    class NET,ESP,UART out
```

## Sensor Simulation Model

```mermaid
flowchart TB
    SS[Sensor Simulator<br/>circular path<br/>500 m radius<br/>Aarhus, Denmark]

    SS --> GPS[GPS<br/>lat/lon/alt/heading<br/>RTK fix, 12 sats]
    SS --> IMU[IMU<br/>accel/gyro/quaternion<br/>banked turn]
    SS --> MAG[Magnetometer<br/>heading + XYZ uT]
    SS --> BARO[Barometer<br/>pressure/altitude/temp]
    SS --> AHT[AHT20<br/>cabin temp/humidity]

    GPS --> KLV[0x0001 Position]
    IMU --> KLV[IMU data for future tags]
    MAG --> KLV[0x0104 Mag Heading]
    BARO --> KLV[0x0102 Baro Altitude]
    AHT --> KLV[0x0103 Temperature]

    classDef sensor fill:#fce4ec,stroke:#880e4f,stroke-width:2px
    class GPS,IMU,MAG,BARO,AHT sensor
```
