# SRT Transport Setup Guide

> **AI hint:** How to enable the (currently incomplete) SRT transport: component install, config in `main.c`, MediaMTX path, ffmpeg test, encryption, stats, troubleshooting. Status: SRT handshake completes but DATA packets rejected by MediaMTX; deprioritized until RTMP baseline works. Related: `docs/rtmp_reference/MEDIAMTX_SRT.md`, `docs/architecture/MEDIAMTX.md`, `AGENTS.md`.

This document explains how to enable SRT (Secure Reliable Transport) for the STANAG S3 Simulator.

## Overview

The simulator now supports multiple transport modes:
- **UDP** (default) - Simple MPEG-TS over UDP
- **SRT** - Reliable, encrypted MPEG-TS over UDP with retransmission
- **ESP-NOW** - Direct peer-to-peer (no WiFi AP needed)
- **Unix Socket** - For QEMU/devcontainer testing

## Enabling SRT

### 1. Add SRT Component

Since there's no official ESP-IDF SRT component in the registry, you have two options:

#### Option A: Use esp-srt component (recommended)
```bash
cd firmware/stanag-s3-simulator
mkdir -p components
cd components
git clone https://github.com/espressif/esp-srt.git srt
```

Then add to `main/CMakeLists.txt`:
```cmake
REQUIRES
    ...
    srt
```

#### Option B: Use Haivision SRT directly (advanced)
```bash
cd firmware/stanag-s3-simulator
mkdir -p components
cd components
git clone https://github.com/haivision/srt.git
# Create components/srt/CMakeLists.txt with proper ESP-IDF component structure
```

### 2. Configure in main.c

Uncomment and modify the SRT configuration section:

```c
/* Example SRT configuration: */
network_tx_set_mode(NETWORK_TX_MODE_SRT);
srt_config_t srt_cfg;
network_tx_srt_config_default(&srt_cfg);
strcpy(srt_cfg.ip, "192.168.4.2");     // MediaMTX/SRT listener IP
srt_cfg.port = 9000;                    // SRT port
srt_cfg.mode = SRT_MODE_CALLER;         // Connect to listener
srt_cfg.latency_ms = 200;               // 200ms buffer
// strcpy(srt_cfg.passphrase, "secret"); // Optional encryption
// srt_cfg.pbkeylen = 16;                // AES-128
network_tx_set_srt_config(&srt_cfg);
```

### 3. Initialize SRT

After `network_tx_init()`, add:
```c
if (network_tx_get_mode() == NETWORK_TX_MODE_SRT) {
    ESP_ERROR_CHECK(network_tx_srt_init());
}
```

## MediaMTX SRT Configuration

MediaMTX supports SRT ingest. Add to `mediamtx.yml`:

```yaml
paths:
  stanag:
    source: srt://:9000?mode=listener&latency=200
    # Or for caller mode:
    # source: srt://192.168.4.1:9000?mode=caller&latency=200
```

## ffmpeg SRT Testing

Receive SRT stream with ffmpeg:
```bash
# Listener mode (simulator is caller)
ffmpeg -i "srt://0.0.0.0:9000?mode=listener&latency=200" -c copy output.ts

# Caller mode (simulator is listener)
ffmpeg -i "srt://192.168.4.1:9000?mode=caller&latency=200" -c copy output.ts
```

## SRT Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `ip` | Remote IP (caller) or bind IP (listener) | "192.168.4.2" |
| `port` | Remote port (caller) or bind port (listener) | 9000 |
| `mode` | CALLER, LISTENER, RENDEZVOUS | CALLER |
| `passphrase` | Encryption passphrase | "" (disabled) |
| `pbkeylen` | Key length: 0, 16, 24, 32 bytes | 0 |
| `latency_ms` | Buffer latency in ms | 200 |
| `enable_tlpktdrop` | Drop too-late packets | true |
| `mss` | Max segment size | 1500 |
| `nonce_enabled` | Use nonce for encryption | false |

## Encryption

Enable AES encryption:
```c
strcpy(srt_cfg.passphrase, "your-secret-passphrase");
srt_cfg.pbkeylen = 16;  // 16=AES-128, 24=AES-192, 32=AES-256
```

Both sender and receiver must use the same passphrase and key length.

## Monitoring SRT Stats

```c
char stats[512];
network_tx_srt_get_stats(stats, sizeof(stats));
ESP_LOGI(TAG, "SRT Stats:\n%s", stats);
```

Output example:
```
SRT Stats:
  Connected: yes
  Packets sent: 15000
  Packets lost: 0
  Packets retransmitted: 12
  Bytes sent: 45000000
  Send buffer (bytes): 128000
  RTT (ms): 45
  Bandwidth (Mbps): 12.50
  Packet send period (us): 3333
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `srt_create_socket` fails | Check SRT component is properly linked |
| Connection timeout | Verify firewall allows UDP port, check IP/port |
| High packet loss | Increase `latency_ms`, check network quality |
| Encryption mismatch | Verify passphrase and `pbkeylen` match on both ends |
| Build error: `srt.h` not found | Ensure SRT component is in `components/srt/` and added to `REQUIRES` |

## Performance Notes

- SRT adds ~1-2ms CPU overhead per frame
- Buffer latency (`latency_ms`) trades off latency vs reliability
- Typical WAN settings: 200-500ms latency
- Typical LAN settings: 50-100ms latency
- MTU/MSS should match network path (1500 for Ethernet, 1400 for WiFi)