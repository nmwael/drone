# WS2812 Status LED Color Cheat Sheet

**GPIO:** 48 (confirmed via color-coded pin scan)  
**LED count:** 2 (LED0 = WiFi state, LED1 = SRT state)  
**Color order:** GRB (Green-Red-Blue) — `set_pixel(g, r, b)`  
**Brightness:** ~50% (CONFIG_STATUS_LED_BRIGHTNESS=128, range 0-255)

---

## LED States

| State | LED 0 (WiFi) | LED 1 (SRT) | Meaning |
|-------|-------------|-------------|---------|
| **BOOT** | Amber blink (200ms on/off) | Amber blink (200ms on/off) | System booting, no network yet |
| **WIFI_WAIT** | Blue slow blink (500ms on/off) | Dark (off) | Connected to AP, waiting for IP |
| **SRT_WAIT** | Green solid | Amber fast blink (500ms on/off) | WiFi OK, SRT handshake in progress |
| **SRT_ON** | Green solid | Green solid | SRT connected, streaming |
| **ERROR** | Red slow double-flash (200ms on/800ms off) | Red slow double-flash | Connection error |
| **OFF** | Dark | Dark | LED disabled |

---

## Quick Color Reference

| Color | RGB (GRB order) | When you see it |
|-------|-----------------|-----------------|
| Green | (255, 0, 0) | SRT connected & streaming ✅ |
| Blue | (0, 0, 255) | WiFi connecting, no IP yet |
| Amber | (180, 60, 0) | Booting or SRT handshaking |
| Red | (0, 255, 0) | Error condition ❌ |
| Off | (0, 0, 0) | LED disabled |

---

## State Transitions

```
POWER ON
   │
   ▼
BOOT (amber blink)
   │
   ├─ WiFi connecting ──► WIFI_WAIT (blue blink)
   │                         │
   │                         └─ Got IP ──►
   │                                 │
   │                         SRT handshake ──► SRT_WAIT (green solid + amber blink)
   │                                 │
   │                                 └─ SRT connected ──► SRT_ON (both green solid)
   │
   └─ Error at any point ──► ERROR (red double-flash)
```

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| LED stays amber (BOOT) | WiFi not connecting | Check SSID/password, AP range |
| LED stays blue (WIFI_WAIT) | No DHCP IP | Check router DHCP, signal strength |
| LED stays amber+green (SRT_WAIT) | SRT handshake failing | Check 172.160.27.93:8890 reachability |
| LED red (ERROR) | Connection error | Check serial logs for details |
| No LED at all | GPIO 48 not wired | Verify WS2812 data pin on your board |

---

## Serial Output Correlation

Watch both the LED **and** serial monitor:

```
I (432) STATUS_LED: WS2812 status strip: 2 LED(s) on GPIO48
I (2792) STANAG_POC: Got IP: 192.168.1.73       → LED turns green (LED0)
I (3522) SRT: SRT handshake complete           → LED1 stops blinking
I (3522) NETWORK_TX_SRT: SRT connected         → Both LEDs solid green
```

---

## File Location

Source: `firmware/stanag-s3-simulator/main/status_led.c`  
Config: `firmware/stanag-s3-simulator/sdkconfig.defaults` (CONFIG_STATUS_LED_GPIO=48)
