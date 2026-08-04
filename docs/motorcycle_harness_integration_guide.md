# AEZEL — Motorcycle Harness Integration & Installation Guide

This document provides a step-by-step installation guide for splicing the **AEZEL VCU** into a motorcycle's electrical harness.

---

## 📑 Table of Contents

- [1. Safety Precautions & Warnings](#1-safety-precautions--warnings)
- [2. Motorcycle Harness Tap Points](#2-motorcycle-harness-tap-points)
- [3. Step-by-Step Installation Instructions](#3-step-by-step-installation-instructions)
  - [3.1 Power & Ground Connections](#31-power--ground-connections)
  - [3.2 Ignition Key Sense Wire](#32-ignition-key-sense-wire)
  - [3.3 Engine RPM Pulse Pickup Wiring](#33-engine-rpm-pulse-pickup-wiring)
  - [3.4 Wheel Speed Hall Sensor Installation](#34-wheel-speed-hall-sensor-installation)
  - [3.5 Fuel Sender Wiring & Resistance Curve](#35-fuel-sender-wiring--resistance-curve)
  - [3.6 Discrete Indicator Lines](#36-discrete-indicator-lines)
- [4. Bench Testing & Pre-Ride Checklist](#4-bench-testing--pre-ride-checklist)

---

## 1. Safety Precautions & Warnings

> [!CAUTION]
> **READ BEFORE WORKING ON MOTORCYCLE WIRING:**
> - Disconnect the **Battery Negative (-) Terminal** before cutting or splicing any wires.
> - Use heat-shrink tubing and waterproof automotive connectors (e.g. Deutsch DT or Superseal 1.5).
> - Never connect 12V harness wires directly to ESP32 GPIOs. Always use the specified opto-isolator circuits.

---

## 2. Motorcycle Harness Tap Points

| AEZEL VCU Input | Motorcycle Wire Function | Recommended Tap Location | Isolation Required |
| :--- | :--- | :--- | :--- |
| **BATT+ (+12V)** | Permanent Battery positive | Main fuse box / Battery + | 5A Inline Fuse |
| **GND** | Battery negative / Chassis ground | Frame ground / Battery - | Star ground connection |
| **IGNITION (GPIO 48)** | Key switch switched +12V | Ignition lock harness / Fuse box | PC817 Opto-Isolator |
| **RPM (GPIO 17)** | Ignition coil negative terminal | HT Coil primary terminal | RC Low-Pass + Opto |
| **SPEED (GPIO 18)** | Hall effect speed sensor | Rear wheel swingarm mount | 3.3V TVS Diode |
| **FUEL (GPIO 1)** | Fuel float sender signal | Fuel tank harness connector | 10k/20k Voltage Divider |
| **LEFT IND (GPIO 19)** | Left turn signal positive | Turn signal relay / Switch pod | PC817 Opto-Isolator |
| **RIGHT IND (GPIO 0)** | Right turn signal positive | Turn signal relay / Switch pod | PC817 Opto-Isolator |
| **NEUTRAL (GPIO 35)** | Neutral switch ground/signal | Engine crankcase neutral switch | PC817 Opto-Isolator |
| **HIGH BEAM (GPIO 36)** | High beam headlight positive | Headlight bucket connector | PC817 Opto-Isolator |

---

## 3. Step-by-Step Installation Instructions

### 3.1 Power & Ground Connections
1. Connect **BATT+** directly to the positive battery terminal through a **5A inline fuse**.
2. Connect **GND** to a clean frame chassis bolt or battery negative terminal.

### 3.2 Ignition Key Sense Wire
1. Locate the wire from the ignition lock harness that shows **+12V when the key is ON** and **0V when the key is OFF**.
2. Route this wire to the PC817 opto-isolator input for GPIO 48 (`PIN_IN_IGNITION`).

### 3.3 Engine RPM Pulse Pickup Wiring
1. Locate the primary (low-voltage) side of the ignition coil.
2. Tap the **coil negative terminal** (the wire running between the CDI/TCI box and the coil).
3. Connect through a $10\text{k}\Omega\text{ 2W}$ series resistor and $100\text{nF}$ capacitor filter to the opto-isolator input for GPIO 17.

### 3.4 Wheel Speed Hall Sensor Installation
1. Mount the Hall effect sensor on the rear swingarm using a rigid bracket.
2. Attach a strong neodymium disc magnet to the rear wheel brake disc rotor bolt.
3. Align the sensor gap to $2\text{mm} - 4\text{mm}$. Connect output to GPIO 18 (`PIN_SPEED_HALL`).

### 3.5 Fuel Sender Wiring & Resistance Curve
1. Tap the single wire coming from the fuel tank float level sender.
2. AEZEL supports standard resistive senders ($10\Omega$ Full / $100\Omega$ Empty, or $33\Omega$ Full / $240\Omega$ Empty).
3. Connect via the 10k/20k voltage divider to GPIO 1 (`PIN_FUEL_SENDER_ADC`).

---

## 4. Bench Testing & Pre-Ride Checklist

Before mounting the unit onto the motorcycle:
- [ ] Validate 5.0V and 3.3V power rails with a digital multimeter.
- [ ] Test ignition ON/OFF key detection (confirm GPIO 48 reads 3.3V when key is ON, 0V when OFF).
- [ ] Confirm WS2812B NeoPixel ring welcome pulse and display splash screen.
- [ ] Verify turn indicator, neutral, and high beam icons respond to 12V harness inputs.
- [ ] Test speed and RPM pulse inputs with a signal generator or manual wheel spin.
- [ ] Verify deep sleep battery current draw is `< 15 µA` when key is turned OFF.
