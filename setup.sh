#!/usr/bin/env bash
# ============================================================================
#  AEZEL — One-Command Installer & Build Script
#  Installs PlatformIO CLI + Wokwi CLI, builds the firmware, and validates Wokwi setup.
# ============================================================================

set -e

GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}====================================================${NC}"
echo -e "${CYAN}   AEZEL — Smart Motorcycle Cockpit Setup Script    ${NC}"
echo -e "${CYAN}====================================================${NC}"

# 1. Ensure Python 3 & Pip are available
echo -e "\n${YELLOW}[1/4] Checking Python environment...${NC}"
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 is not installed.${NC}"
    exit 1
fi

# 2. Install / Verify PlatformIO CLI
echo -e "\n${YELLOW}[2/4] Installing / verifying PlatformIO CLI...${NC}"
if ! command -v pio &> /dev/null; then
    python3 -m pip install platformio --quiet --user || pip install platformio --quiet
    # Add ~/.local/bin and python bin paths if needed
    export PATH="$HOME/.local/bin:$HOME/.python/current/bin:$PATH"
fi

if command -v pio &> /dev/null; then
    echo -e "${GREEN}✓ PlatformIO version: $(pio --version)${NC}"
elif [ -f "$HOME/.python/current/bin/pio" ]; then
    export PATH="$HOME/.python/current/bin:$PATH"
    echo -e "${GREEN}✓ PlatformIO version: $(pio --version)${NC}"
else
    echo -e "${RED}Error: Failed to locate 'pio' executable.${NC}"
    exit 1
fi

# 3. Install / Verify Wokwi CLI
echo -e "\n${YELLOW}[3/4] Installing / verifying Wokwi CLI...${NC}"
if ! command -v wokwi-cli &> /dev/null && [ ! -f "$HOME/bin/wokwi-cli" ]; then
    curl -sL https://wokwi.com/ci/install.sh | sh
fi
export PATH="$HOME/bin:$PATH"
if command -v wokwi-cli &> /dev/null || [ -f "$HOME/bin/wokwi-cli" ]; then
    echo -e "${GREEN}✓ Wokwi CLI ready.${NC}"
fi

# 4. Build AEZEL Firmware
echo -e "\n${YELLOW}[4/4] Compiling AEZEL Firmware (PlatformIO esp32-phoenix)...${NC}"
pio run -e esp32-phoenix

if [ -f ".pio/build/esp32-phoenix/firmware.bin" ]; then
    echo -e "\n${GREEN}====================================================${NC}"
    echo -e "${GREEN}   BUILD SUCCESSFUL!                                ${NC}"
    echo -e "${GREEN}   Firmware Binary: .pio/build/esp32-phoenix/firmware.bin ${NC}"
    echo -e "${GREEN}====================================================${NC}"
else
    echo -e "\n${RED}Build failed: firmware.bin not generated.${NC}"
    exit 1
fi

# 5. Optional Upload or Simulation instructions
if [ "$1" == "--upload" ]; then
    echo -e "\n${CYAN}Uploading firmware to target hardware...${NC}"
    pio run -e esp32-phoenix -t upload
elif [ "$1" == "--sim" ]; then
    echo -e "\n${CYAN}Running Wokwi simulation...${NC}"
    if [ -z "$WOKWI_CLI_TOKEN" ] && [ -f "$HOME/.wokwi/user.tok" ]; then
        export WOKWI_CLI_TOKEN=$(grep '^key=' "$HOME/.wokwi/user.tok" | cut -d'=' -f2)
    fi
    wokwi-cli --timeout 15000 .
fi

echo -e "\n${CYAN}To flash an ESP32 board directly: ${YELLOW}./setup.sh --upload${NC}"
echo -e "${CYAN}To run Wokwi simulation:           ${YELLOW}./setup.sh --sim${NC}"
