# Systematic Debugging Skill

Use when a build, flash, or runtime behavior fails and the cause is not immediately obvious.

1. Reproduce the failure and capture the exact error output (build log, QEMU monitor output, or serial log).
2. For build failures: read the failing file and surrounding context; check `sdkconfig.defaults` for required Kconfig options (`CONFIG_SPIRAM=y`, `CONFIG_PARTITION_TABLE_CUSTOM=y`).
3. For runtime failures on hardware: capture serial output with `stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0` and parse with `python3 scripts/parse_ts_output.py serial.log`.
4. Formulate one hypothesis at a time; change one thing; re-run to verify.
5. If blocked after two attempts, return a Result with `status: blocked` and the exact error as the blocker; do not loop.

Common firmware fixes (from `TROUBLESHOOTING.md` / `BUILD.md`):
- SPIFFS image fails → `CONFIG_PARTITION_TABLE_CUSTOM=y` in `sdkconfig.defaults`
- `esp_psram_*` undefined → `CONFIG_SPIRAM=y` in `sdkconfig.defaults`
- `idf.py` not found → run `eim shell v5.5.6`
- UDP errno 118 (WSL/QEMU) → expected; use `network_tx_set_unix_socket("/tmp/stanag.sock")`
- Frame overrun >33 ms → profile with `ESP_LOGI` at frame boundaries; reduce frame rate or optimize
