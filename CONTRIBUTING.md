# Contributing to Drone Monorepo

Thank you for contributing! This monorepo contains firmware, ground station, and documentation for an RC truck autonomy project with STANAG 4609 video streaming.

## Quick Links

- **Active Firmware:** `firmware/stanag-s3-simulator/` — Currently running on hardware
- **AI Onboarding:** `AGENTS.md` (root) + `firmware/stanag-s3-simulator/AGENTS.md`
- **Quick Reference:** `QUICK-REFERENCE.md` (root) + `firmware/stanag-s3-simulator/QUICK-REFERENCE.md`

---

## Development Workflow

### 1. Setup Devcontainer
```bash
# Open in VS Code / Cursor with Dev Containers extension
# Or use Podman/Docker directly
# ESP-IDF v5.5.6 auto-sourced via EIM
eim shell v5.5.6
```

### 2. Make Changes
- Follow existing code style (ESP-IDF clang-format)
- Update documentation alongside code changes
- Add tests for new functionality

### 3. Test
```bash
# STANAG S3 Simulator
cd firmware/stanag-s3-simulator
idf.py build                          # Build
idf.py -DTEST_COMPONENT=1 build       # Build tests
timeout 60 idf.py -p /dev/null qemu monitor  # Run tests on QEMU
```

### 4. Submit PR
- Clear title and description
- Link related issues
- Include test results
- Update `CHANGELOG.md` (if exists)

---

## Code Standards

### C Code (ESP-IDF)
- **Style:** ESP-IDF default (`idf.py format` uses clang-format)
- **Naming:** `snake_case` for functions/variables, `PascalCase` for types
- **Prefix:** Module prefix for public APIs (e.g., `jpeg_loader_`, `sensor_sim_`)
- **Error handling:** Return `esp_err_t`, use `ESP_ERROR_CHECK()` for init
- **Logging:** `ESP_LOGI(TAG, ...)` with module-specific TAG
- **Memory:** Static buffers for hot path, PSRAM for large allocations

### Documentation
- **Format:** Markdown with Mermaid diagrams
- **Location:** Component-level docs in each firmware directory
- **API changes:** Update `API.md` when changing function signatures
- **Diagrams:** Update `OVERVIEW.md`/`DATA-FLOW.md` for architecture changes

### Testing
- **Framework:** Unity (built into ESP-IDF)
- **Location:** `test/<module>_test.c`
- **Registration:** Add `RUN_TEST()` in `test/test_runner.c`
- **Target:** QEMU for CI, hardware for validation

---

## Subsystem Guidelines

### STANAG S3 Simulator (`firmware/stanag-s3-simulator/`)
- **Target:** ESP32-S3 with PSRAM
- **Pipeline:** 30 fps JPEG→H.264→KLV→MPEG-TS→UDP
- **Key files:** `main/main.c`, `main/stanag_types.h`, component `.c/.h` pairs
- **Tests:** 64 Unity tests in `test/`

### ESP32-S3 Vision (`firmware/esp32-s3-vision/`) — Planned
- **Target:** ESP32-S3 with PSRAM + OV5640 camera
- **Pipeline:** CSI camera → HW H.264 → KLV → MPEG-TS → ESP-NOW
- **New components:** `h264_encoder.c`, `klv_builder.c`, `ts_muxer.c`, `wifi_stream.c`

### ESP32-C3 Sensor Hub (`firmware/esp32-c3-hub/`) — Planned
- **Target:** ESP32-C3
- **Sensors:** GPS, IMU, Mag, Baro, AHT20, BMP280
- **Fusion:** EKF for sensor fusion
- **Comms:** ESP-NOW to S3, UART to RP2040s, UART to ESP8266

### RP2040 ToF (`firmware/rp2040-tof-{1,2,3}/`) — Planned
- **Target:** RP2040 (16MB/4MB flash variants)
- **Sensors:** 4× VL53L5CX per board via I2C mux
- **Output:** UART streaming to C3 hub

### Ground Station (`ground-station/`)
- **MediaMTX:** `ground-station/mediamtx/` — Go media server config
- **WebXR App:** `ground-station/webxr-app/` — Three.js + WebXR + WebCodecs

---

## Pull Request Checklist

- [ ] Code follows style (`idf.py format` passes)
- [ ] Build passes (`idf.py build`)
- [ ] Tests pass (`idf.py -DTEST_COMPONENT=1 build && qemu test`)
- [ ] Documentation updated (`API.md`, `OVERVIEW.md`, `DATA-FLOW.md`)
- [ ] No new warnings (check build output)
- [ ] Binary size reasonable (check `idf.py size-components`)
- [ ] Commit messages clear and atomic

---

## Reporting Issues

Use GitHub/Codeberg issues with:
1. **Clear title** — Component + symptom
2. **Environment** — ESP-IDF version, target, host OS
3. **Reproduction steps** — Minimal steps to reproduce
4. **Expected vs actual** — What should happen vs what happens
5. **Logs** — Serial output, build output, test output
6. **Configuration** — Relevant `sdkconfig` settings

---

## License

By contributing, you agree that your contributions will be licensed under the MIT License (same as project).