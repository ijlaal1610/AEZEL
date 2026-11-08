# GitHub Sponsors Profile Setup Guide for @ijlaal1610

Use this copy-paste-ready guide to customize your [GitHub Sponsors Dashboard](https://github.com/sponsors/dashboard) (or [Sponsors Application](https://github.com/sponsors/signup)).

---

## 1. Short Description / Bio (under 140 characters)

```
Building AEZEL — the universal open-source connected vehicle cockpit & VCU (Apple CarPlay, Android Auto & ESP32-P4 RISC-V).
```

---

## 2. Introduction & Story (Markdown Profile Body)

Paste this directly into your **GitHub Sponsors Profile Description / Story**:

```markdown
### Hey there! I'm Ijlaal Akhtar 👋

I'm an embedded systems engineer and open-source builder from India. I believe connected vehicle technology—digital instrument clusters, telemetry, and smart vehicle control—should be open, accessible, and community-driven, rather than locked behind proprietary, expensive OEM walled gardens.

---

### 🏍️ What I'm Building: AEZEL

I am the creator and lead maintainer of **[AEZEL](https://github.com/ijlaal1610/AEZEL)**: a universal, open-source connected cockpit platform and Vehicle Control Unit (VCU) designed for:
- 🏍️ **Motorcycles**: Manual transmission gear detection, lean angle calculation, chain maintenance, side-stand interlocks.
- 🛵 **Scooters**: CVT transmission, belt wear intervals, centrifugal clutch math.
- 🚗 **Cars & EVs**: PRND automatic, dual G-force crash sensing, and automotive CAN Bus / OBD-II telemetry.

AEZEL spans across **Three Hardware Tiers**:
1. **Tier 1 (ESP32-S3 Lite)**: 60 FPS LVGL cluster with BLE Turn-by-Turn HUD mirroring real Google & Apple Maps.
2. **Tier 2 (ESP32-P4 Auto)**: Direct MCU-based Wireless Android Auto on 400 MHz RISC-V silicon with VCU overlays.
3. **Tier 3 (Raspberry Pi Pro)**: Full In-Vehicle Infotainment with genuine Apple CarPlay & Android Auto via LIVI.

---

### 🎯 Why Your Sponsorship Directly Matters

Unlike pure software web applications, building an automotive embedded platform requires physical silicon, high-voltage test benches, and sensor hardware:

- 🔬 **ESP32-P4 RISC-V Silicon**: Procuring evaluation boards and 800×480 MIPI-DSI ST7701 touch displays for native wireless Android Auto projection.
- 🏍️ **Motorcycle Test Harnesses**: Oscilloscope validation rigs, 12V transient surge TVS clamping, and opto-isolated signal filters to test against ignition-coil noise safely.
- 📡 **Dual-Band GNSS Modules**: Procuring u-blox NEO-M8N / M9N satellite receivers and active patch antennas.
- 🔌 **CAN Bus Transceivers**: MCP2515 controllers and TJA1050 / SN65HVD230 transceivers for real ECU reverse-engineering.

Every single cent contributed goes directly into real physical hardware prototyping, keeping AEZEL 100% free, open-source under the Apache 2.0 license, and accessible to riders, builders, and EV innovators worldwide.

---

### 🤝 What Sponsors Receive

- 💖 **Public Recognition**: Your name or avatar featured on the official AEZEL repository README and project website.
- 🚀 **Early Access**: Direct preview of upcoming firmware releases, Flutter companion app builds, and hardware schematics.
- 🛠️ **Direct Collaboration**: For higher tiers, direct 1-on-1 consultation on your custom motorcycle, scooter, or EV wiring harness.

Thank you for fueling open-source vehicle mobility! ⚡
```

---

## 3. Recommended Sponsorship Tiers

### Monthly Tiers

| Tier | Amount | Title | Description & Perks |
| :--- | :--- | :--- | :--- |
| **Tier 1** | **$3 / mo** | ☕ Coffee Fuel | *Buy a coffee to power late-night FreeRTOS task optimizations & sensor calibration.*<br>• Mentioned in release changelogs<br>• Sponsor badge on your GitHub profile |
| **Tier 2** | **$10 / mo** | ⚡ Spark Plug | *Funds discrete sensor components, voltage regulators, and PCB prototypes.*<br>• Your GitHub avatar & name on the AEZEL README Sponsor Wall<br>• Priority bug triage in GitHub Issues |
| **Tier 3** | **$25 / mo** | 🔌 CAN Bus Backer | *Directly funds automotive CAN transceivers and high-precision u-blox GNSS modules.*<br>• Priority review for custom vehicle preset profiles (gear ratios, fuel curves)<br>• Early beta access to Flutter companion app releases |
| **Tier 4** | **$50 / mo** | 🔬 RISC-V Hardware Champion | *Directly funds ESP32-P4 automotive evaluation boards & MIPI-DSI displays.*<br>• Direct 1-on-1 Discord/Telegram consultation on your custom EV/bike wiring & harness pinouts<br>• Prominently featured in all major releases |
| **Tier 5** | **$100 / mo** | 🏎️ Cockpit Visionary (Corporate) | *Significant backer helping accelerate full Apple CarPlay & Android Auto milestones.*<br>• Large logo & backer link on the AEZEL README and official website (`ijlaal1610.github.io/AEZEL/`)<br>• Direct architectural input on our roadmap milestones |

---

### One-Time Tiers

- **$5** — *Digital Wave*: One-time micro-tip for project appreciation.
- **$25** — *Sensor Supporter*: One-time contribution towards discrete sensor prototypes.
- **$50** — *Hardware Booster*: One-time contribution towards ESP32-P4 silicon dev kits.
- **$100** — *Bench Testing Patron*: One-time contribution towards automotive harness test equipment.

---

## 4. Sponsorship Goals to Set in Dashboard

1. **Goal 1: $100 / month — Automotive Test Bench & Harness Lab**
   > *Allows continuous testing on physical motorcycle harnesses with 12V transient surge testing and oscilloscope validation.*
2. **Goal 2: $300 / month — ESP32-P4 RISC-V Hardware Lab**
   > *Funds continuous hardware procurement of ESP32-P4 RISC-V chips, MIPI-DSI displays, and CAN test nodes to fast-track Tier 2 Wireless Android Auto.*
