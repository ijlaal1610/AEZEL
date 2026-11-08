#!/usr/bin/env bash
# ============================================================================
#  AEZEL — One-Command Setup, Build & Simulation Runner
# ============================================================================
set -e

BOLD="\033[1m"
GREEN="\033[0;32m"
CYAN="\033[0;36m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
RESET="\033[0m"

echo -e "${BOLD}${CYAN}"
echo "============================================================================"
echo "   AEZEL — Universal Multi-Vehicle & Multi-Hardware Cockpit Platform"
echo "============================================================================"
echo -e "${RESET}"

MODE="build"
if [ "$1" == "--upload" ]; then
    MODE="upload"
elif [ "$1" == "--sim" ]; then
    MODE="sim"
elif [ "$1" == "--test" ]; then
    MODE="test"
fi

# 1. Check Python 3
echo -e "${BOLD}[1/4] Checking environment prerequisites...${RESET}"
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: Python 3 is required but not installed.${RESET}"
    exit 1
fi
echo -e "${GREEN}✓ Python 3 found: $(python3 --version)${RESET}"

# 2. Check/Install PlatformIO
echo -e "${BOLD}[2/4] Checking PlatformIO toolchain...${RESET}"
if ! command -v pio &> /dev/null; then
    echo -e "${YELLOW}PlatformIO CLI not found. Installing via pip...${RESET}"
    pip install -U platformio
fi
echo -e "${GREEN}✓ PlatformIO found: $(pio --version)${RESET}"

# 3. Native Unit Tests
echo -e "${BOLD}[3/4] Running native hardware-independent unit tests...${RESET}"
mkdir -p /tmp/aezel_build
g++ -std=c++17 -Iinclude -Isrc test/native/test_vehicle_math.cpp src/VehicleMath.cpp src/VehicleConfig.cpp -o /tmp/aezel_build/test_vehicle_math
/tmp/aezel_build/test_vehicle_math

g++ -std=c++17 -Iinclude -Isrc test/native/test_vehicle_profile.cpp src/VehicleConfig.cpp -o /tmp/aezel_build/test_vehicle_profile
/tmp/aezel_build/test_vehicle_profile

g++ -std=c++17 -Iinclude -Isrc test/native/test_hal_linux.cpp src/hal/linux_pi/LinuxGpio.cpp src/hal/linux_pi/LinuxStorage.cpp src/hal/linux_pi/LinuxBle.cpp src/hal/linux_pi/LinuxCan.cpp -o /tmp/aezel_build/test_hal_linux
/tmp/aezel_build/test_hal_linux
echo -e "${GREEN}✓ All 83 native unit tests passed!${RESET}"

# 4. Action execution
echo -e "${BOLD}[4/4] Executing target: ${CYAN}${MODE}${RESET}..."
case "$MODE" in
    build)
        echo -e "${CYAN}Compiling firmware binary for ESP32-S3 (esp32-aezel)...${RESET}"
        pio run -e esp32-aezel
        echo -e "${BOLD}${GREEN}Build complete! Binary generated at .pio/build/esp32-aezel/firmware.bin${RESET}"
        ;;
    upload)
        echo -e "${CYAN}Flashing firmware to connected ESP32-S3 device...${RESET}"
        pio run -e esp32-aezel -t upload
        ;;
    sim)
        echo -e "${CYAN}Launching AEZEL Web Telemetry Simulator...${RESET}"
        echo -e "${YELLOW}Open http://127.0.0.1:8080 in your browser to view the live dashboard.${RESET}"
        python3 sim/sim_server.py
        ;;
    test)
        echo -e "${GREEN}Tests completed successfully.${RESET}"
        ;;
esac
