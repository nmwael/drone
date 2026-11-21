# MediaMTX RTMP Source Reference

> **AI hint:** Reference for MediaMTX's RTMP implementation (conn.go/server.go/session.go): handshake sequence, chunk stream IDs, AMF command order, FLV video tag format, constants. Use when comparing ESP32 wire format against MediaMTX expectations. Related: `RTMP_DEBUG_JOURNAL.md`, `RTMP_E2E_TEST_PLAN.md`, `main/network_tx_rtmp.c`.

## Repository
**URL:** https://github.com/bluenviron/mediamtx (formerly github.com/aler9/mediamtx)

## Relevant Files

### RTMP Protocol Implementation
| File | Path in Repository | Purpose |
|------|-------------------|---------|
| rtmp/conn.go | rtmp/conn.go | RTMP connection handling, handshake |
| rtmp/server.go | rtmp/server.go | RTMP server listener |
| rtmp/session.go | rtmp/session.go | RTMP session management |
| rtmp/conn.go (Publish) | rtmp/conn.go | RTMP publish command handling |
| rtmp/conn.go (PublishStart) | rtmp/conn.go | Publish start handling |

### Key RTMP Functions in MediaMTX
- rtmpConn.readChunk() - Read RTMP chunks
- rtmpConn.writeChunk() - Write RTMP chunks
- rtmpConn.readMessage() - Read RTMP messages
- rtmpConn.writeMessage() - Write RTMP messages
- rtmpConn.readMessage() - Parse RTMP AMF commands
- rtmpConn.onPublish() - Handle RTMP publish command

## Key Implementation Details from FFmpeg Source

### RTMP Handshake Sequence (C0+C1 → S0+S1+S2 → C2)
1. Client sends C0 (1 byte version 3) + C1 (1536 bytes random)
2. Server responds with S0 (1 byte) + S1 (1536 bytes) + S2 (1536 bytes)
3. Client sends C2 (1536 bytes copy of S1)

### RTMP Chunk Stream IDs
| CSID | Name | Usage |
|------|------|-------|
| 2 | RTMP_CHUNK_PROTOCOL_CONTROL | Connection control (set chunk size, window ACK, set peer BW) |
| 3 | RTMP_CHUNK_INVOKE | AMF commands (connect, createStream, publish) |
| 4 | RTMP_CHUNK_SOURCE | Publish stream |
| 5 | RTMP_CHUNK_VIDEO | Video data |

### RTMP AMF Commands Sequence (Publishing)
1. connect - Connect to application
2. createStream - Create a stream
3. publish - Publish to stream
4. Video data chunks follow

### RTMP Video Packet Format (FLV Tag)
RTMP video messages are wrapped in FLV tag format:
- Byte 0: 0x09 (video tag type)
- Bytes 1-3: data size (big-endian)
- Bytes 4-6: timestamp (big-endian)
- Byte 7: timestamp extended (0)
- Bytes 8-10: stream ID (0)
- FLV video body:
  - Byte 0: 0x17 (keyframe + AVC)
  - Bytes 1-3: composition time offset (0x000000)
  - Bytes 4+: NAL unit data

### Key RTMP Constants
```
RTMP_CHUNK_PROTOCOL_CONTROL = 2
RTMP_CHUNK_INVOKE = 3
RTMP_CHUNK_SOURCE = 4
RTMP_CHUNK_VIDEO = 5
RTMP_PT_INVOKE = 0x14 (20)
RTMP_PT_VIDEO = 0x09 (9)
RTMP_PT_CHUNK_SIZE = 0x01 (1)
RTMP_PT_WINDOW_ACK_SIZE = 0x05 (5)
RTMP_PT_SET_PEER_BW = 0x06 (6)
RTMP_PT_USER_CONTROL = 0x04 (4)
```

## Key Implementation Notes
- RTMP uses AMF0 (Action Message Format 0) for command encoding
- All multi-byte integers are big-endian
- Stream ID in chunk header is 3 bytes little-endian
- MediaMTX expects proper RTMP handshake (C0+C1 → S0+S1+S2 → C2)
- MediaMTX expects client to send Window Ack Size, Set Peer BW, User Control Stream Begin, Chunk Size after connect
- MediaMTX expects _result response to connect and publish commands
- Video data must be FLV-wrapped H.264 NAL units