# Remote MediaMTX Validation Plan

> **AI hint:** End-to-end validation checklist for streaming from ESP32 to a *remote* (production) MediaMTX instance at 172.160.27.93:1935. Read when testing against production or reproducing the "works locally, fails remotely" class of issues. Related: `RTMP_E2E_TEST_PLAN.md` (local/loopback focus), `docs/rtmp_reference/RTMP_DEBUG_JOURNAL.md` (wire-format history), `AGENTS.md` (transport status).

**Target:** 172.160.27.93:1935 (production)  
**Local Reference:** 127.0.0.1:1935 (debug)  
**Firmware:** ESP32-S3, all RTMP wire-format fixes applied  
**Publish Target:** `rtmp://sse_drone_f71919@172.160.27.93:1935/live/sse_drone_f71919`

---

## Prerequisites (Complete Before Starting)

- [ ] Local MediaMTX auth fixed and verified working
- [ ] Network reachability to 172.160.27.93:1935 confirmed (✅ done)
- [ ] ESP-IDF v5.5.6 sourced (`eim shell v5.5.6`)

---

## Step 1: Validate Local MediaMTX with ffmpeg

```bash
# Test publish to local (matching firmware URL format exactly)
ffmpeg -re -f lavfi -i testsrc2=size=1280x720:rate=30 \
  -c:v libx264 -preset ultrafast -tune zerolatency \
  -f flv "rtmp://sse_drone_f71919@127.0.0.1:1935/live/sse_drone_f71919"
```

**Success Criteria:**
- [ ] Connection establishes (handshake + connect + createStream + publish)
- [ ] Video frames accepted (no "Connection reset" / ECONNRESET)
- [ ] Stream appears in MediaMTX API: `curl http://localhost:9997/v3/paths/list`

---

## Step 2: Sync Remote MediaMTX Config

```bash
# Copy working local config to remote
scp ground-station/mediamtx/mediamtx.yml user@172.160.27.93:/path/to/mediamtx.yml

# Restart remote MediaMTX
ssh user@172.160.27.93 "systemctl restart mediamtx"  # or docker restart
```

**Critical Config Sections to Match:**
- [ ] `auth` section (publish credentials)
- [ ] `paths.live` section (stream key validation)
- [ ] `protocols.rtmp` port 1935
- [ ] `hls` port 8888 enabled

---

## Step 3: Test ffmpeg Against Remote

```bash
# Exact same URL firmware will use
ffmpeg -re -f lavfi -i testsrc2=size=1280x720:rate=30 \
  -c:v libx264 -preset ultrafast -tune zerolatency \
  -f flv "rtmp://sse_drone_f71919@172.160.27.93:1935/live/sse_drone_f71919"
```

**Success Criteria:**
- [ ] TCP connect succeeds (no timeout)
- [ ] RTMP handshake completes (C0/C1/S0/S1/S2)
- [ ] Authentication accepted (connect response _result)
- [ ] createStream returns stream ID (typically 1)
- [ ] publish succeeds (onStatus "NetStream.Publish.Start")
- [ ] Video data frames accepted (no ECONNRESET)
- [ ] Remote API shows stream: `curl http://172.160.27.93:9997/v3/paths/list`

---

## Step 4: Build & Flash ESP32 Firmware

```bash
cd /workspaces/drone/firmware/stanag-s3-simulator
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

**Note:** Firmware uses hardcoded config matching above URL. No code changes needed if Steps 1-3 pass.

---

## Step 5: Monitor Serial for 7 RTMP Stages

Watch for these log sequences (TAG: `NET_TX_RTMP`):

| Stage | Expected Log |
|-------|--------------|
| 1. TCP Connect | `RTMP: Connected to 172.160.27.93:1935` |
| 2. Handshake | `RTMP: Handshake complete (C0/C1/S0/S1/S2)` |
| 3. Connect (chunk stream 3) | `RTMP: Connect sent, awaiting _result` |
| 4. Connect Result | `RTMP: Connect _result received` |
| 5. createStream (chunk stream 3) | `RTMP: createStream sent, stream_id=X` |
| 6. Publish (chunk stream 4, stream_id=X) | `RTMP: Publish sent for live/sse_drone_f71919` |
| 7. Publish Result | `RTMP: Publish _result / onStatus NetStream.Publish.Start` |
| 8. **First Video Frame** | `RTMP: Video frame sent (type=1, len=XXXX, csid=4)` |

**Success Criteria:**
- [ ] All 8 log stages appear in order
- [ ] No `ECONNRESET` / `errno 104` / `Connection reset by peer`
- [ ] No `RTMP: Send failed` errors

---

## Step 6: Verify No ECONNRESET on First Video Frame

**Critical Check:** After "Publish _result", the first video packet must be accepted.

```bash
# In serial monitor, look for:
RTMP: Video frame sent (type=1, len=XXXX, csid=4, timestamp=XXXXX)
# NOT:
RTMP: Send failed: errno 104 (Connection reset by peer)
```

**If ECONNRESET occurs:**
- Remote MediaMTX rejected the video packet format
- Compare local vs remote MediaMTX logs
- Verify FLV video tag format matches ffmpeg reference

---

## Step 7: Test HLS Playback at Remote :8888

```bash
# From any browser/client:
# HLS manifest: http://172.160.27.93:8888/live/sse_drone_f71919/index.m3u8
# Or test with ffplay:
ffplay "http://172.160.27.93:8888/live/sse_drone_f71919/index.m3u8"
```

**Success Criteria:**
- [ ] Manifest loads (200 OK)
- [ ] Segments appear (.ts files listed)
- [ ] Video plays in ffplay/browser (30fps, 1280x720)
- [ ] KLV metadata present in TS (verify with `scripts/parse_ts_output.py` if capturing)

---

## Go/No-Go Decision Matrix

| Check | Pass | Fail Action |
|-------|------|-------------|
| Step 1 local ffmpeg | ✅ | Fix local MediaMTX auth first |
| Step 3 remote ffmpeg | ✅ | Compare configs; check remote logs |
| Step 5 all 7 stages | ✅ | Analyze which stage fails; check wire format |
| Step 6 no ECONNRESET | ✅ | Video packet format mismatch; check FLV tags |
| Step 7 HLS plays | ✅ | Check HLS config; segment generation |

---

## Rollback / Debug Commands

```bash
# Remote MediaMTX logs
ssh user@172.160.27.93 "journalctl -u mediamtx -f"  # or docker logs -f mediamtx

# Remote API status
curl http://172.160.27.93:9997/v3/paths/list

# Capture ESP32 serial for analysis
cat /dev/ttyACM0 > esp32_remote_test.log
python3 scripts/parse_ts_output.py esp32_remote_test.log
```

---

## Sign-Off

- [ ] All 7 steps completed successfully
- [ ] ESP32 streams continuously for >60 seconds without disconnect
- [ ] HLS playback verified on remote
- [ ] KLV telemetry parsed and valid

**Validated By:** _______________ **Date:** _______________