# Security Policy

## 🛡️ Security Overview

AEZEL runs on embedded hardware connected directly to motorcycle electrical systems, BLE companion devices, and external sensors. Security and safety are critical priorities for our software architecture.

---

## ⚠️ Supported Versions

Security updates and fixes are applied to the `main` branch.

| Version | Supported |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

---

## 🔒 Reporting a Vulnerability

If you discover a security vulnerability or a safety-critical bug within AEZEL (such as a memory corruption issue that could freeze the FreeRTOS watchdog, BLE unauthorized command injection, or improper power rail shutdown logic):

1. **Do NOT open a public GitHub issue.**
2. Send a private report to the maintainers at **`ijlaalakhtar@gmail.com`**.
3. Include:
   - Description of the vulnerability or safety issue.
   - Affected subsystem manager or code location.
   - Proof-of-concept steps or hardware conditions to reproduce.
   - Recommended patch or mitigation strategy (if known).

---

## 🚨 Security Best Practices for Motorcyclists

- **Isolated Power Rail**: Always install a TVS diode and reverse-polarity diode ahead of the buck converter powering the ESP32-S3.
- **Opto-Isolators**: Never bypass opto-isolation on 12V harness inputs (ignition, indicators, kill switch, starter).
- **BLE Authentication**: AEZEL's BLE manager uses Passkey Bonding + MITM protection for remote commands. Never disable NimBLE security settings when extending the companion API.
