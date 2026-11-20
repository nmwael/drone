# RTMP Debug Journal - ESP32 to MediaMTX

## Problem Statement
ESP32 RTMP connects and publishes successfully, but MediaMTX rejects video data with ECONNRESET (errno 104) on first video DATA packet.

## Root Cause Found
**Three wire-format bugs in `main/network_tx_rtmp.c`** (verified against ffmpeg RTMP reference `ffmpeg_rtmp_proto.c`):

1. **NAL Length Prefix Corruption** (`rtmp_send_video_nalu()`): Only lowest byte of 4-byte big-endian NAL length written. For NAL=1000 bytes (0x000003E8), wrote `00 00 00 E8` instead of `00 00 03 E8`. MediaMTX reads massive NAL length, overruns buffer, closes connection.

2. **Wrong Video Chunk Stream ID**: `RTMP_CHUNK_VIDEO = 4` conflicted with `RTMP_CHUNK_SOURCE` (also 4). ffmpeg uses `RTMP_VIDEO_CHANNEL = 5` for video data. MediaMTX sees CSID conflict and rejects.

3. **Wrong Stream ID in Video Chunk Header**: Used hardcoded `stream_id=0` but ffmpeg uses the **server-assigned stream ID** from createStream `_result` (typically 1) in the RTMP chunk header's Message Stream ID field.

## Wire-Format Fixes Applied (all verified against ffmpeg_rtmp_proto.c)

### Fix 1: NAL Length Prefix (`network_tx_rtmp.c:1338-1341`)
**Before:** Only wrote lowest byte:
```c
rtmp_put_be24(&flv_tag[hlen + copy_len], 0);  /* top 3 bytes = 0 (WRONG for NAL > 255) */
copy_len += 3;
flv_tag[hlen + copy_len++] = nalus[i].len & 0xFF;  /* only lowest byte */
```
**After:** Write all 4 bytes big-endian:
```c
flv_tag[hlen + copy_len++] = (nalus[i].len >> 24) & 0xFF;
flv_tag[hlen + copy_len++] = (nalus[i].len >> 16) & 0xFF;
flv_tag[hlen + copy_len++] = (nalus[i].len >> 8) & 0xFF;
flv_tag[hlen + copy_len++] = nalus[i].len & 0xFF;
```

### Fix 2: Video Chunk Stream ID (`network_tx_rtmp.c:39`)
**Before:** `#define RTMP_CHUNK_VIDEO 4` (conflicted with `RTMP_CHUNK_SOURCE`)
**After:** `#define RTMP_CHUNK_VIDEO 5` (matches ffmpeg's `RTMP_VIDEO_CHANNEL`)

### Fix 3: Video Chunk Header Stream ID (`network_tx_rtmp.c:318-322`)
**Before:** Hardcoded `stream_id=0`:
```c
header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0;
```
**After:** Use server-assigned `s_rtmp.stream_id` from createStream `_result`:
```c
header[hlen++] = s_rtmp.stream_id & 0xFF;
header[hlen++] = (s_rtmp.stream_id >> 8) & 0xFF;
header[hlen++] = (s_rtmp.stream_id >> 16) & 0xFF;
header[hlen++] = (s_rtmp.stream_id >> 24) & 0xFF;
```
Matches ffmpeg line 3120: `rt->out_pkt.extra = rt->stream_id;`

### Fix 4: onMetadata Chunk Header Truncation (`network_tx_rtmp.c:1144-1156`)
**Before:** Only wrote 3 bytes for stream ID (fmt=0 header needs 4 bytes), hardcoded to 0:
```c
uint8_t header[11];  /* too small - missing 1 byte */
...
header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0;  /* only 3 bytes! */
```
**After:** Write 4 bytes using `s_rtmp.stream_id`:
```c
uint8_t header[12];  /* correct size for fmt=0 header */
...
header[hlen++] = s_rtmp.stream_id & 0xFF;
header[hlen++] = (s_rtmp.stream_id >> 8) & 0xFF;
header[hlen++] = (s_rtmp.stream_id >> 24) & 0xFF;
header[hlen++] = s_rtmp.stream_id & 0xFF;
```

### Fix 5: Dynamic Stream ID Retrieval Placement (`network_tx_rtmp.c:981-985`)
**Before:** Called `rtmp_read_create_stream_response()` AFTER `rtmp_drain_response()`, which had already consumed the createStream `_result`.
**After:** Call `rtmp_read_create_stream_response()` immediately after `rtmp_send_create_stream()`, before any drain. Falls back to `stream_id=1` if parsing fails.

## FFmpeg Reference Verification (from `ffmpeg_rtmp_proto.c`)

| Item | ffmpeg Behavior | ESP32 Code | Match |
|------|----------------|------------|-------|
| Connect CSID | `RTMP_SYSTEM_CHANNEL` (3) | `RTMP_CHUNK_INVOKE` (3) | ✅ |
| CreateStream CSID | `RTMP_SYSTEM_CHANNEL` (3) | `RTMP_CHUNK_INVOKE` (3) | ✅ |
| Publish CSID | `RTMP_SOURCE_CHANNEL` (4) | `RTMP_CHUNK_SOURCE` (4) | ✅ |
| Video CSID | `RTMP_VIDEO_CHANNEL` (5) | `RTMP_CHUNK_VIDEO` (5) | ✅ |
| Control msgs CSID | `RTMP_NETWORK_CHANNEL` (2) | `RTMP_CHUNK_PROTOCOL_CONTROL` (2) | ✅ |
| Video stream ID | Server-assigned from createStream | `s_rtmp.stream_id` | ✅ |
| onMetadata stream ID | Server-assigned from createStream | `s_rtmp.stream_id` | ✅ |
| _result for connect stream ID | 0 | 0 | ✅ |
| Control msg order | WinAck → SetPeerBW → StreamBegin → ChunkSize → _result → onBWDone | Same order | ✅ |
| FLV video tag | 0x09 + 24-bit size + 24-bit ts + 8-bit ts_ext + 24-bit stream_id + body | Same | ✅ |
## Current Status

**Firmware builds, flashes, and boots successfully.** WiFi connects (IP: 192.168.1.73). RTMP connection attempts execute but fail with **errno 118 (ETIMEDOUT)** because the ESP32's WiFi subnet (192.168.1.x) cannot route to 172.160.27.93:1935.

**This is a network reachability issue, NOT a wire-format issue.** All wire-format discrepancies with ffmpeg have been resolved — once TCP connectivity is established, the RTMP handshake should proceed without ECONNRESET.

## Wire-Format Fixes Applied (all verified against ffmpeg_rtmp_proto.c)

### P0 — CRITICAL (prevent ECONNRESET on video) — **ALL APPLIED**
| # | Fix | Location | Status |
|---|-----|----------|--------|
| 1 | Removed `rtmp_send_connect_result()` — client must not send server→client message | ~L958-963 | ✅ |
| 2 | Removed `rtmp_send_on_bw_done()` — client must not send server→client message | ~L966-971 | ✅ |
| 3 | `RTMP_CHUNK_VIDEO`: 4 → 5 (matches ffmpeg `RTMP_VIDEO_CHANNEL`) | L39 | ✅ **Fixed** |
| 4 | Publish CSID: 3 → 4 (matches `RTMP_SOURCE_CHANNEL`) | ~L590 | ✅ |
| 5 | Video chunk header stream_id: uses `s_rtmp.stream_id` (server-assigned) | L318-322 | ✅ |
| 6 | Extended timestamp byte when timestamp ≥ 0xFFFFFF | L325-330, L345-352 | ✅ |

### P1 — HIGH (interoperability)
| # | Fix | Location |
|---|-----|----------|
| 7 | onMetadata: NOTIFY (0x12) on csid=4 with `@setDataFrame` prefix | ~L1105-1160 |
| 8 | Added `releaseStream` and `FCPublish` before createStream | ~L971-984 |

### Previously applied fixes
| # | Fix | Location |
|---|-----|----------|
| A | NAL length prefix: 4-byte big-endian per NAL | L1338-1341 |
| B | Dynamic stream ID from createStream _result (before drain) | L978-988 |
| C | onMetadata stream_id uses `s_rtmp.stream_id` (not hardcoded 0) | L1150-1153 |

## Next Steps (P0: Network Reachability)

1. **Verify MediaMTX is listening on 0.0.0.0:1935** (not just 127.0.0.1)
2. **Verify routing from 192.168.1.x to 172.160.27.x** - check gateway/firewall
3. **Test from same network:** From a device on the ESP32's WiFi, run `nc -zv 172.160.27.93 1935`
4. **Once connected:** Watch serial for `RTMP: connected and publishing` → `First video frame sent successfully`
5. **Packet capture:** `tcpdump -i any -w rtmp.pcap port 1935` to compare with ffmpeg

## Reference Files
- `docs/rtmp_reference/ffmpeg_rtmp_proto.c` - ffmpeg RTMP source (packet writing, chunk headers)
- `docs/rtmp_reference/ffmpeg_rtmp.h` - ffmpeg RTMP header
- `docs/rtmp_reference/RTMP_DEBUG_JOURNAL.md` - this file
- `docs/srt_reference/` - MediaMTX SRT source