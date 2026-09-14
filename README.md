# EFS USB-C Sink Controller

A web-controlled, dark-mode terminal interface for an ESP32-C3 microcontroller coupled with an AP33772S USB-C PD 3.1 sink controller and an INA219 current/voltage monitor. Designed for precision power delivery and real-time telemetry logging.

## Features

* **Configurable Power Delivery**: Dynamically request standard fixed voltages (3.3V, 5.0V, 12.0V, 15.0V, 20.0V) and Programmable Power Supply (16.0V PPS) via the AP33772S controller.
* **Dual Telemetry Path**: Automatically switches between INA219 high-side telemetry (for the 3.3V LDO regulated rail) and AP33772S internal ADCs (for 5V–20V rails).
* **Dynamic Wi-Fi AP**: Automatically broadcasts a unique network identifier prefixed with `EFS_USBC_Sink_` and an 8-character MAC hex suffix.
* **Retro Terminal Web UI**: Responsive mobile-friendly dark-mode dashboard hosted locally via LittleFS with live polling, current limits, and error interlocks.

---

## Hardware Pinout (ESP32-C3)

| Component / Function | ESP32-C3 Pin |
| --- | --- |
| **I2C SDA** | `IO4` (SDA_3V3) |
| **I2C SCL** | `IO5` (SCL_3V3) |
| **Power Output Enable** | `IO6` (`OUT_EN` / `PWR_EN`) |

---

## File Structure

```text
├── platformio.ini
├── data
│   ├── index.html
│   ├── style.css
│   └── script.js
└── src
    └── main.cpp

```

---

## Getting Started & Flashing

1. Clone or open this project directory within **Visual Studio Code** with the **PlatformIO** extension installed.
2. Connect your ESP32-C3 board via USB.
3. Build and flash the C++ firmware:
```bash
pio run --target upload

```


4. Build and flash the LittleFS filesystem image (containing the web UI assets):
```bash
pio run --target buildfs
pio run --target uploadfs

```



---

## Usage

1. Reset the ESP32-C3. It will broadcast an open Wi-Fi access point named `EFS_USBC_Sink_[HEX]`.
2. Connect to the Wi-Fi network using password `warg_efs`.
3. Open a web browser and navigate to `[http://192.168.4.1](http://192.168.4.1)`.
4. Use the terminal interface to select your target voltage, adjust the current limit slider, and toggle the output state.
