# ESP32 Advanced GPS Tracking Suite: Cloud & Local Architectures

##  Overview
This repository contains two distinct embedded C++ firmware solutions for real-time GPS tracking, utilizing an ESP32 microcontroller and a Ublox NEO-6M GPS module. 

Instead of a single approach, this project explores two different IoT system architectures:
1. **Cloud-Based Telemetry (Firebase):** An internet-dependent pipeline that pushes continuous JSON payloads to a global database for remote monitoring.
2. **Edge-Computing Web Server (Local LAN):** A standalone system that hosts its own HTTP server and dynamically generates an HTML dashboard, requiring no internet access or cloud fees.

## Shared Hardware Architecture
Both solutions utilize the exact same physical hardware and wiring configuration.

* **Microcontroller:** ESP32 Development Board
* **Sensor:** Ublox NEO-6M Serial GPS Module
* **Power:** 3.3V / 5V (Depending on GPS module variant)

### Pin Configuration
| ESP32 Pin | GPS Module Pin | Function |
| :--- | :--- | :--- |
| `GPIO 16` (RX2) | `TX` | Receives NMEA data from GPS |
| `GPIO 17` (TX2) | `RX` | Sends commands to GPS |
| `3V3` / `VIN` | `VCC` | Power supply |
| `GND` | `GND` | Common Ground |

---

##  Architecture 1: Firebase Cloud Tracker
*(Located in the `/Firebase_Tracker` directory)*

This firmware parses raw NMEA data and securely streams continuous telemetry to a Firebase Realtime Database. It is designed for assets that need to be tracked globally from a remote location.

### Key Features
* **Non-Blocking Architecture:** Parses serial GPS data continuously while managing Wi-Fi and Firebase uploads asynchronously.
* **Auto-Reconnection Logic:** Actively monitors Wi-Fi status and reconnects without stalling the main loop.
* **Structured Data:** Pushes a bundled JSON payload containing latitude, longitude, altitude, speed, and satellites.
