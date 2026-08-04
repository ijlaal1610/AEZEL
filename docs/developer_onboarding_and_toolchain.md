# AEZEL — Developer Onboarding & Toolchain Guide

This document provides a step-by-step onboarding guide for configuring the development environment, compiling firmware, running Wokwi hardware simulations, and contributing to **AEZEL**.

---

## 📑 Table of Contents

- [1. Prerequisites](#1-prerequisites)
- [2. Environment Setup](#2-environment-setup)
- [3. Compiling Firmware](#3-compiling-firmware)
- [4. Wokwi Simulation Setup & Verification](#4-wokwi-simulation-setup--verification)
- [5. One-Command `./setup.sh` Workflow](#5-one-command-setupsh-workflow)
- [6. Flashing Physical ESP32 Hardware](#6-flashing-physical-esp32-hardware)

---

## 1. Prerequisites

- **Operating System**: Linux (Ubuntu/Debian recommended), macOS, or Windows WSL2.
- **Python**: Python 3.9+ with `pip` and `venv`.
- **Git**: Git 2.30+.
- **VS Code** (Optional): With PlatformIO IDE extension and Wokwi extension installed.

---

## 2. Environment Setup

### 1. Clone Repository
```bash
git clone https://github.com/ijlaal1610/AEZEL.git
cd AEZEL
```

### 2. Install PlatformIO CLI
```bash
python3 -m pip install -U platformio
```

### 3. Install Wokwi CLI (For Hardware Simulation)
```bash
curl -L https://wokwi.com/ci/install.sh | sh
```

---

## 3. Compiling Firmware

AEZEL uses PlatformIO to manage libraries, toolchains, and build flags.

### Build Binary Target:
```bash
pio run -e esp32-phoenix
```

The compiled output artifacts will be generated in:
- `.pio/build/esp32-phoenix/firmware.bin` (Firmware binary)
- `.pio/build/esp32-phoenix/firmware.elf` (ELF symbol file)

---

## 4. Wokwi Simulation Setup & Verification

AEZEL includes a virtual testbench mapped to ESP32-S3 DevKitC-1 pins:

- [`diagram.json`](file:///workspaces/AEZEL/diagram.json): Virtual schematic (ILI9341 display, rotary encoder, push buttons, potentiometers, MPU6050, DS18B20, buzzer, and NeoPixel strip).
- [`wokwi.toml`](file:///workspaces/AEZEL/wokwi.toml): Simulation configuration pointing to `.pio/build/esp32-phoenix/firmware.bin`.

### Lint Simulation Schematic:
```bash
wokwi-cli lint
```

### Run Cloud Hardware Simulation (Requires Token):
```bash
export WOKWI_CLI_TOKEN="your_wokwi_token_here"
wokwi-cli . --timeout 5000
```

---

## 5. One-Command `./setup.sh` Workflow

AEZEL includes an automated setup script that handles all installation, compilation, and simulation steps in one command:

```bash
# Make executable and run default build
chmod +x setup.sh
./setup.sh

# Build & Run Simulation
./setup.sh --sim

# Build & Flash Connected ESP32 Hardware
./setup.sh --upload
```

---

## 6. Flashing Physical ESP32 Hardware

Connect your ESP32-S3 DevKitC-1 via USB-C and run:

```bash
pio run -e esp32-phoenix -t upload
```

To monitor serial console output (115200 Baud):
```bash
pio device monitor -b 115200
```
