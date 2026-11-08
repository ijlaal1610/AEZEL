# Contributing to AEZEL

Thank you for your interest in contributing to **AEZEL**! AEZEL is an open-source ESP32-S3 smart motorcycle platform built to transform standard motorcycles into connected, intelligent vehicles.

---

## 🛠️ How to Contribute

### 1. Reporting Bugs
- Search existing [GitHub Issues](https://github.com/ijlaal1610/AEZEL/issues) before opening a new issue.
- Clearly describe the bug, including steps to reproduce, hardware setup, board revision, and full serial output logs.

### 2. Suggesting Features
- Open an Issue titled `[Feature Request] <Short Description>`.
- Explain the use case, electrical/software implications, and how it aligns with AEZEL's VCU architecture.

### 3. Code Contributions
1. **Fork the Repository** and clone your fork locally.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-new-manager
   ```
3. **Follow the Architectural Rules**:
   - **Central Pin Map**: All GPIO pin assignments MUST be declared strictly in `include/Config.h`. Never hardcode pin numbers in manager code.
   - **Thread Safety**: Never access raw globals across FreeRTOS tasks. All state mutations must go through `SharedState::instance().update(...)` or `snapshot()`.
   - **Subsystem Decoupling**: Subsystem managers (e.g. `DisplayManager`, `BleManager`) must remain completely decoupled from raw GPIO pin reads. Sensor acquisition belongs strictly in `SensorManager`.
   - **Safety First**: Non-critical background tasks (SD writes, BLE I/O, GPS UART) belong on Core 0 (`CORE_CONNECTIVITY`), keeping Core 1 (`CORE_REALTIME`) available for 60 FPS UI rendering and safety warnings.

4. **Verify the Build & Simulation**:
   ```bash
   ./setup.sh
   /home/codespace/bin/wokwi-cli lint .
   ```
5. **Submit a Pull Request**:
   - Ensure your PR description clearly summarizes the changes, hardware testing performed, and links to relevant issues.

---

## 🎨 Code Style & Standards

- **Language**: C++17 / C for ESP32 Arduino framework.
- **Naming Conventions**:
  - Class names: `CamelCase` (e.g., `LightingManager`)
  - Subsystem files: `src/managers/SubsystemManager.cpp`
  - Member variables: `_camelCase` with leading underscore (e.g., `_lastRenderMs`)
  - Enums / Flags: `UPPER_SNAKE_CASE` or `CamelCase`
- **Formatting**: Clean 4-space indentation, clear doc comments explaining non-obvious hardware workarounds.

---

## 📜 Code of Conduct

All contributors are expected to uphold our [Code of Conduct](CODE_OF_CONDUCT.md) in all interactions within the AEZEL project.
