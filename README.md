# STM32 Advanced RC Transmitter 🎮📡

![PlatformIO](https://img.shields.io/badge/PlatformIO-Build-orange?logo=platformio)
![STM32](https://img.shields.io/badge/Hardware-STM32F103-blue?logo=stmicroelectronics)
![C++](https://img.shields.io/badge/Code-C%2B%2B-00599C?logo=c%2B%2B)

A professional-grade, 8-channel RC transmitter firmware built for the **STM32F103C8 (Blue Pill)**. 
Features a responsive OLED interface, non-blocking menu system, custom mixing, and reliable NRF24L01+ communication.

---

## ✨ Features

### 🎮 Control & Input
- **8 Channels:** 4 Analog (Gimbals) + 2 Analog (Pots) + 2 Digital (Switches).
- **Digital Trims:** Persistent trim adjustment for Roll, Pitch, and Yaw saved to EEPROM.
- **Custom Mapping:** `Border_Map` algorithm for precise stick calibration (Split-curve mapping).

### 🖥️ User Interface (OLED 0.96")
- **Real-time Dashboard:** Displays Battery Voltage, Timer, and Channel Bars.
- **Flight Timer:** Countdown/Count-up timer with buzzer alerts.
- **Menu System:** Adjust settings (Buzzer, Dark Mode, Inversion) directly on the device.
- **Splash Screen:** Custom animated boot sequence (MIG-21 Jet).

### ⚙️ Hardware & Reliability
- **Non-blocking Core:** Uses state machines for buttons and buzzer (no `delay()` in loop).
- **Battery Monitor:** 2S/3S LiPo voltage monitoring with Low-Voltage Alarm.
- **External EEPROM:** Uses 24LCxx via I2C for reliable settings storage.
- **High-Speed Radio:** NRF24L01+ configured at 250kbps for maximum range and stability.

---

## 📂 Project Structure

```text
STM32-RC-Transmitter/
├── include/              # Header files (if separated)
├── lib/                  # External libraries
├── src/                  # Source Code & Headers
│   ├── main.cpp          # Entry point & Main Loop
│   ├── Radio.cpp/.h      # NRF24L01 Driver & Logic
│   ├── DisplayManager... # OLED UI & Graphics Engine
│   ├── Button.cpp/.h     # Non-blocking Input Handler
│   ├── EEPROM_...        # Manual I2C Storage Driver
│   └── Settings.h        # Global Configuration Structs
├── test/                 # Unit testing (PlatformIO default)
├── platformio.ini        # Build & Board Configuration
├── LICENSE               # MIT License
└── README.md             # Documentation
```
---

## 🔌 Pinout Configuration

| Component | STM32 Pin | Description |
| :--- | :--- | :--- |
| **Radio (NRF24)** | | |
| CE | `PB8` | Chip Enable |
| CSN | `PB9` | Chip Select Not |
| SCK/MISO/MOSI | `PA5/PA6/PA7` | SPI1 Bus |
| **Display (OLED)** | | |
| SDA / SCL | `PB7 / PB6` | I2C1 Bus |
| **EEPROM** | | |
| SDA / SCL | `PB11 / PB10` | I2C2 Bus |
| **Controls** | | |
| Gimbals (A/E/T/R) | `PA0-PA3` | ADC Inputs |
| Aux Pots | `PB0, PB1` | ADC Inputs |
| Aux Switches | `PB4, PB5` | Digital Inputs |
| **Buttons** | | |
| Trims | `PB15, PA8-PA10, PA15, PB3` | Active Low |
| Navigation | `PB12-PB14` | Up/Down/Enter |
| **Misc** | | |
| Buzzer | `PC13` | Active High |
| V-Sense | `PA4` | Voltage Divider Input |

---

## 🔌 Hardware Setup Notes

For stable operation, please ensure the following hardware configurations:

### 1. NRF24L01+ Module
- **Power:** This module is extremely sensitive to power noise.
  - Solder a **10µF to 100µF capacitor** directly across the VCC and GND pins of the module.
  - Use a dedicated 3.3V regulator (like AMS1117-3.3) if possible, as the STM32's onboard 3.3V might not provide enough peak current.

### 2. I2C Bus (Display & EEPROM)
- **Pull-up Resistors:** The STM32 Blue Pill requires external pull-up resistors (4.7kΩ) on `SCL` and `SDA` lines for both I2C1 and I2C2 buses, as internal pull-ups are weak.
- **EEPROM Addressing:** For 24LCxx chips:
  - Connect pins A0, A1, A2 to **GND** to set the I2C address to `0x50`.
  - Ensure the WP (Write Protect) pin is connected to **GND** to enable writing.

### 3. Buzzer
- **Driver Circuit:** Do not connect the buzzer directly to the GPIO. Use a **NPN Transistor (e.g., 2N2222)** or a MOSFET driver circuit to protect the microcontroller pin.

### 4. Battery Voltage Divider
- The voltage divider ratio used in code is `R1=22kΩ` (to Battery +) and `R2=6.8kΩ` (to GND).
- Maximum measurable voltage: ~14V (Safe for 3S LiPo).

---

## 🚀 Getting Started

### Prerequisites
- **VS Code** with **PlatformIO** extension.
- **STM32F103 (Blue Pill)** board.
- **ST-Link V2** programmer.

### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/EbrSiami/STM32-RC-Transmitter.git
   ```

2. Open the project folder in PlatformIO.

3. Build the firmware:
    Hit the **Checkmark (✓)** icon in the bottom bar.

4. Upload to board:
    Connect ST-Link and hit the **Arrow (→)** icon.

---

## ❤️ Dedication & Acknowledgements

**Special dedication to Marya.**

This project is built upon logic and lines of code that you wrote. Thank you for your patience and for the hidden messages I found too late. You were the co-pilot of this project even when I was flying solo.

*   **Lead Developer:** Ebrahim Siami
*   **Core Logic & Inspiration:** Marya

- **Libraries:** Powered by ```Adafruit_SSD1306```, ```RF24```, and ```Wire```.

---

*This project is open-source under the MIT License.*
