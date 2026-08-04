# AEZEL — Hardware Schematics & Pinout Specification

This document provides an exhaustive hardware schematic reference, pinout mapping, component selection guide, and electrical protection specification for the **AEZEL** Smart Motorcycle Cockpit and Vehicle Control Unit (VCU) platform.

---

## 📑 Table of Contents

- [1. ESP32-S3 Complete Pinout Table](#1-esp32-s3-complete-pinout-table)
- [2. Electrical Protection & Voltage Conditioning](#2-electrical-protection--voltage-conditioning)
  - [2.1 Power Tree & Buck Converter](#21-power-tree--buck-converter)
  - [2.2 Discrete Harness Signal Conditioning (Opto-Isolators)](#22-discrete-harness-signal-conditioning-opto-isolators)
  - [2.3 Analog ADC Voltage Dividers & TVS Clamping](#23-analog-adc-voltage-dividers--tvs-clamping)
  - [2.4 Ignition Coil RPM Pickup Low-Pass Filter](#24-ignition-coil-rpm-pickup-low-pass-filter)
- [3. Bus Interfaces & Pin Assignments](#3-bus-interfaces--pin-assignments)
  - [3.1 Display & Touch SPI Bus](#31-display--touch-spi-bus)
  - [3.2 SD Card Mass Storage SPI Bus](#32-sd-card-mass-storage-spi-bus)
  - [3.3 I2C Sensor Bus (IMU, Barometer, RTC)](#33-i2c-sensor-bus-imu-barometer-rtc)
  - [3.4 1-Wire Temperature Sensor Bus](#34-1-wire-temperature-sensor-bus)
  - [3.5 GPS Module UART Interface](#35-gps-module-uart-interface)

---

## 1. ESP32-S3 Complete Pinout Table

The table below lists every GPIO pin assignment configured in [`include/Config.h`](file:///workspaces/AEZEL/include/Config.h), along with electrical characteristics, internal pull-up/pull-down states, and peripheral function:

| GPIO | Function Name | Direction | Electrical Type | Protection Circuit | Notes / Description |
| :---: | :--- | :---: | :--- | :--- | :--- |
| **0** | `PIN_IN_RIGHT_INDICATOR` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Right turn signal harness read |
| **1** | `PIN_FUEL_SENDER_ADC` | Input | Analog ADC1_CH0 | 10k/20k Divider + 3.3V TVS | Fuel float sender resistive voltage |
| **2** | `PIN_BATTERY_ADC` | Input | Analog ADC1_CH1 | 40k/10k Divider + 18V TVS | Main battery voltage (0–18V range) |
| **3** | `PIN_CHARGE_ADC` | Input | Analog ADC1_CH2 | 40k/10k Divider + 18V TVS | Regulator charging line voltage |
| **4** | `PIN_ROTARY_A` | Input | Digital (Pull-up) | RC Debounce | Rotary Encoder Phase A |
| **5** | `PIN_ROTARY_B` | Input | Digital (Pull-up) | RC Debounce | Rotary Encoder Phase B |
| **6** | `PIN_ROTARY_SW` | Input | Digital (Pull-up) | RC Debounce | Rotary Encoder Push Button |
| **7** | `PIN_BTN_MODE` | Input | Digital (Pull-up) | RC Debounce | Physical Mode / Back Button |
| **8** | `PIN_BTN_OK` | Input | Digital (Pull-up) | RC Debounce | Physical Confirmation / OK Button |
| **9** | `PIN_TFT_BL` | Output | PWM (LEDC CH0 5kHz) | N-FET Gate Driver | TFT Backlight PWM Auto-Brightness |
| **10** | `PIN_TFT_CS` | Output | Digital | Direct 3.3V SPI | TFT Display Chip Select (Active Low) |
| **11** | `PIN_TFT_DC` | Output | Digital | Direct 3.3V SPI | TFT Data/Command Control |
| **12** | `PIN_TFT_RST` | Output | Digital | Direct 3.3V SPI | TFT Hardware Reset (Active Low) |
| **13** | `PIN_TOUCH_CS` | Output | Digital | Direct 3.3V SPI | Touch Controller Chip Select |
| **14** | `PIN_TOUCH_IRQ` | Input | Digital (Pull-up) | Direct 3.3V | Touch Controller Pen Interrupt |
| **15** | `PIN_ONEWIRE_BUS` | I/O | Digital (Open-Drain) | 4.7kΩ Pull-up to 3.3V | Dual DS18B20 1-Wire Temp Bus |
| **16** | `PIN_LIGHT_SENSOR_ADC` | Input | Analog ADC2_CH5 | Divider / LDR | Ambient Light Sensor Analog Read |
| **17** | `PIN_RPM_PICKUP` | Input | Interrupt (Falling) | Opto + RC Low-Pass Filter | Coil-negative pulse timing (1 pulse/rev) |
| **18** | `PIN_SPEED_HALL` | Input | Interrupt (Falling) | Internal Pull-Up + 3.3V TVS | Rear wheel hall pulse timing |
| **19** | `PIN_IN_LEFT_INDICATOR` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Left turn signal harness read |
| **20** | `I2C_SCL` | Output | I2C Clock | 4.7kΩ Pull-up to 3.3V | Shared Sensor I2C Bus SCL |
| **21** | `I2C_SDA` | I/O | I2C Data | 4.7kΩ Pull-up to 3.3V | Shared Sensor I2C Bus SDA |
| **35** | `PIN_IN_NEUTRAL` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Neutral Gear switch harness read |
| **35** | `PIN_OUT_RGB_ACCENT_DATA` | Output | Digital (800kHz NRZ) | 330Ω Series Resistor | WS2812B NeoPixel Accent Data |
| **36** | `PIN_IN_HIGH_BEAM` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | High Beam headlight harness read |
| **36** | `PIN_OUT_BUZZER` | Output | Digital / PWM | N-FET Gate Driver | Piezo Warning Buzzer Driver |
| **37** | `PIN_IN_HORN_SW` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Horn switch harness read |
| **38** | `PIN_SD_MOSI` | Output | Digital (SPI MOSI) | Shared SPI Bus | SD Card / TFT SPI Data Out |
| **39** | `PIN_SD_MISO` | Input | Digital (SPI MISO) | Shared SPI Bus | SD Card / TFT SPI Data In |
| **40** | `PIN_SD_SCK` | Output | Digital (SPI Clock) | Shared SPI Bus | SD Card / TFT SPI Clock |
| **41** | `PIN_IN_SIDE_STAND` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Side Stand switch harness read |
| **42** | `PIN_IN_FRONT_BRAKE` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Front Brake lever switch read |
| **43** | `PIN_GPS_TX` | Output | UART1 TX | Direct 3.3V | GPS Module UART Command Out |
| **44** | `PIN_GPS_RX` | Input | UART1 RX | Direct 3.3V | GPS Module NMEA Sentence In |
| **45** | `PIN_IN_REAR_BRAKE` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Rear Brake pedal switch read |
| **46** | `PIN_IN_CLUTCH` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Clutch lever switch read |
| **47** | `PIN_IN_KILL_SWITCH` | Input | Digital (3.3V Logic) | PC817 Opto-Isolator | Handlebar Kill switch read |
| **47** | `PIN_OUT_DRL_PWM` | Output | PWM (LEDC CH1 5kHz) | N-FET Gate Driver | Daytime Running Light PWM Output |
| **48** | `PIN_IN_IGNITION` | Input | Digital (EXT0 Wake) | PC817 Opto-Isolator | Key Ignition sense & Deep sleep wake |
| **48** | `PIN_OUT_HAZARD_RELAY` | Output | Digital | N-FET / Relay Driver | Hazard Relay Actuator Output |

---

## 2. Electrical Protection & Voltage Conditioning

### 2.1 Power Tree & Buck Converter
Motorcycle electrical systems experience severe voltage fluctuations:
- **Nominal Battery Voltage**: 12.6V DC
- **Charging Voltage**: 13.8V – 14.5V DC
- **Cranking Dip**: Drops to 8.5V DC during starter motor engagement.
- **Load-Dump Transient Spikes**: Can exceed +40V to +60V DC when inductive loads (starter solenoid, alternator regulator) disconnect.

#### Protection Circuitry Schematics:
```
+12V Batt ---> [Reverse Diode 1N5408] ---> [TVS Diode SMAJ24A] ---> [Automotive Buck TPS5430] ---> 5.0V / 3.3V Rails
```
- **1N5408 Reverse Polarity Diode**: Blocks negative voltage if battery terminals are hooked up backward.
- **SMAJ24A TVS Diode**: Clamps positive voltage transients above 24V to protect the buck converter IC.

---

### 2.2 Discrete Harness Signal Conditioning (Opto-Isolators)
To isolate 12V motorcycle harness signals from 3.3V ESP32 GPIO pins, every discrete input (Indicators, Brakes, High Beam, Neutral, Kill Switch, Ignition) uses a **PC817 Opto-Isolator**:

```
+12V Harness Input ---> [1.2kΩ 1W Resistor] ---> [PC817 Anode (Pin 1)]
                                                 [PC817 Cathode (Pin 2)] ---> Harness GND

+3.3V VCC ----------> [10kΩ Pull-Up] -----------> [PC817 Collector (Pin 4)] ---> ESP32 GPIO
                                                 [PC817 Emitter (Pin 3)]   ---> ESP32 GND
```

When 12V is applied to the harness input, the internal infrared LED illuminates, pulling the collector output down to `0V` (`LOW`). When 12V is removed, the 10kΩ resistor pulls the GPIO up to `3.3V` (`HIGH`).

---

### 2.3 Analog ADC Voltage Dividers & TVS Clamping

Battery voltage sensing (0–18V) uses a 40kΩ / 10kΩ resistive voltage divider paired with a 3.3V Zener/TVS diode across the ESP32 ADC input:

```
+12V Battery Sense ---> [40kΩ 1% Resistor] ───┬───> [10kΩ 1% Resistor] ───> GND
                                              ├───> [3.3V TVS Diode]    ───> GND
                                              └───> ESP32 ADC GPIO 2
```

- **Divider Ratio**: $R_{\text{ratio}} = \frac{40\text{k} + 10\text{k}}{10\text{k}} = 5.0$
- **Max Input Voltage**: $3.3\text{V} \times 5.0 = 16.5\text{V}$ (safe headroom up to 18V transient).

---

### 2.4 Ignition Coil RPM Pickup Low-Pass Filter

The RPM pickup line connects to the ignition coil negative terminal, which generates high-voltage inductive kickback spikes (+100V) every time the spark plug fires:

```
Coil Negative ---> [10kΩ 2W Resistor] ---> [100nF Cap to GND] ---> [PC817 Opto-Isolator] ---> ESP32 GPIO 17
```

The RC low-pass filter ($f_c \approx 159\text{ Hz}$) attenuates high-frequency ringing while passing clean 0–200 Hz ignition pulses up to 12,000 RPM.

---

## 3. Bus Interfaces & Pin Assignments

### 3.1 Display & Touch SPI Bus
- **Controller**: ILI9341 / ILI9488 480x320 TFT panel
- **SPI Bus**: SCK (GPIO 40), MOSI (GPIO 38), MISO (GPIO 39)
- **Control Pins**: CS (GPIO 10), D/C (GPIO 11), RST (GPIO 12), BL PWM (GPIO 9)
- **Touch Pins**: Touch CS (GPIO 13), Touch IRQ (GPIO 14)

### 3.2 SD Card Mass Storage SPI Bus
- **Shares Main SPI Bus**: SCK (GPIO 40), MOSI (GPIO 38), MISO (GPIO 39)
- **Dedicated Chip Select**: SD CS (GPIO 5)

### 3.3 I2C Sensor Bus (IMU, Barometer, RTC)
- **Pins**: SDA (GPIO 21), SCL (GPIO 20) with 4.7kΩ pull-up resistors to 3.3V.
- **Connected I2C Devices**:
  - `0x68`: MPU6050 6-Axis Motion Sensor (Accelerometer / Gyroscope)
  - `0x76` / `0x77`: BMP280 Barometric Pressure / Altitude Sensor
  - `0x68`: DS3231 Real-Time Clock (RTC)

### 3.4 1-Wire Temperature Sensor Bus
- **Pin**: GPIO 15 with a 4.7kΩ pull-up resistor to 3.3V.
- **Sensors**: Dual Dallas DS18B20 digital temperature sensors (Engine head & Ambient outside air).

### 3.5 GPS Module UART Interface
- **Module**: NEO-6M / NEO-8M GPS receiver
- **UART Port**: UART1 at 9600 Baud
- **Pins**: ESP32 TX (GPIO 43) -> GPS RX; ESP32 RX (GPIO 44) -> GPS TX
