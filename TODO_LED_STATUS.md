# TODO: Fix WS2812 LED Status Indicator

## Issue
The status LED pin scan runs on every boot (~12 seconds), printing:
```
=== WS2812 PIN SCAN: 32 pins, ~400ms each (~12s) ===
[scan] GPIO 0 done
...
[scan] GPIO 47 done  ← This is the actual LED
=== PIN SCAN COMPLETE ===
Tell me which GPIO number lit up and I will set STATUS_LED_GPIO to it.
```

## Root Cause
`CONFIG_STATUS_LED_SCAN=y` is enabled in sdkconfig (or via menuconfig), causing the LED driver to scan all GPIOs on startup instead of using the configured pin.

## Fix
1. Disable pin scan in menuconfig:
   ```
   idf.py menuconfig
   → Component config → Status LED → [ ] Scan for LED pin
   ```
2. Or set `CONFIG_STATUS_LED_SCAN=n` in `sdkconfig.defaults`
3. Ensure `CONFIG_STATUS_LED_GPIO=47` is set (the actual WS2812 GPIO)

## Files to Update
- `firmware/stanag-s3-simulator/sdkconfig.defaults` - add `CONFIG_STATUS_LED_SCAN=n`
- Verify `CONFIG_STATUS_LED_GPIO=47` is set

## Expected Result
- Boot time reduced by ~12 seconds
- LED immediately shows status (WiFi connecting → blue, SRT connecting → yellow, SRT streaming → green)