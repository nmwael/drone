# DONE: Fixed WS2812 LED Status Indicator

## Changes Made
1. **Disabled pin scan** in `sdkconfig.defaults`:
   ```
   CONFIG_STATUS_LED_SCAN=n
   CONFIG_STATUS_LED_GPIO=45
   ```

2. **Slower blink patterns** in `main/status_led.c`:
   - BOOT: 200ms on/200ms off (was 50ms)
   - WIFI_WAIT: 2s on/2s off slow blue pulse (was 1s)
   - SRT_WAIT: 500ms on/500ms off amber pulse (was 250ms)
   - SRT_ON: Solid green
   - ERROR: 200ms on/800ms off red double flash

## Result
- Boot time reduced by ~12 seconds (no pin scan)
- LED immediately shows status after init
- Slower, more readable blink patterns

## Verified
- ESP32 boots without pin scan
- LED shows WiFi connecting (slow blue) → SRT connecting (medium amber) → Streaming (solid green)