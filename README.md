# ExpressLRS Backpack VRX Emulator for ESP32-S3 Super Mini

An open-source emulation of the **ExpressLRS Backpack** module for video receiver (VRX) channel switching, specifically designed for the **ESP32-S3 Super Mini** board.

This project enables your ESP32-S3 Super Mini to wirelessly receive channel switching commands from your **ELRS TX Module** via ESP-NOW, just like an official VRX Backpack would, while offering a web interface to configure video channels, RC channel thresholds, power levels, and hardware pinouts.

---

## 📡 How It Works (Communication Flow)

1. **EdgeTX Handset (e.g., Radiomaster TX16S)** runs the official ExpressLRS LUA script.
2. In the LUA script, navigate to the **Backpack** folder and set **Telemetry** to **ESPNOW** (or use the VTX Administrator to change the VTX channel).
3. The handset's internal **ELRS TX Module** forwards the CRSF RC channels and MSP packets to its **TX Backpack** chip.
4. The **TX Backpack** broadcasts these frames via **ESP-NOW** to nearby devices.
5. Our **ESP32-S3 Super Mini** acts as an emulator of the VRX Backpack. It uses the same **6-byte UID** derived from your **Binding Phrase** as its MAC address, allowing it to seamlessly catch and decrypt the ESP-NOW packets.
6. The ESP32-S3 decodes the packed 11-bit CRSF channels (or incoming MSP `MSP_SET_VTX_CONFIG` commands) and issues SPI bit-bang commands to change the frequency of your video receiver module.

---

## 🔌 Hardware Pin Connections (ESP32-S3 to VRX/RTC6715)

The emulator controls the VRX module (e.g., RTC6715 or SPI-hacked RX5808 / Rapidfire) via standard 3-wire SPI bit-banging.

### Default Pinout
| Function | ESP32-S3 GPIO Pin | VRX / RTC6715 Module Pad |
| :--- | :--- | :--- |
| **CLK (Clock)** | **GPIO 4** | CLK / CH3 |
| **DATA (Data)** | **GPIO 5** | DATA / CH1 |
| **CS (Chip Select)** | **GPIO 6** | CS / CH2 / SPI_SS |
| **GND** | **GND** | GND |
| **VCC** | **5V / 3.3V** | VCC |

*Note: If your hardware uses different pins, you can easily change them via the Web UI.*

---

## 💻 Web Configuration Interface

The emulator hosts an open Access Point (AP) for easy configuration from your smartphone, PC, or FPV goggles.

* **SSID**: `ELRS_Backpack_VRX_S3`
* **Password**: None (Open)
* **IP Address**: `192.168.4.1`

### Web UI Features:
* **Live Status**: Real-time display of the currently tuned band, channel, frequency, and ESP-NOW packet reception status.
* **RC Channel Bars**: Real-time visual monitoring of your **6POS Switch** and **S2 continuous pot** (displays microsecond values) to verify that your radio transmitter is communicating with the ESP.
* **Binding Phrase**: Enter your ELRS binding phrase; the Web UI calculates the MD5-based 6-byte UID in real-time and applies it.
* **WiFi TX Power Control**: Since the ESP32-S3 Super Mini board can suffer from stability/brownout issues at max WiFi power, the power is reduced below the maximum 75% limit (defaulting to **11 dBm** or ~50% power) to guarantee perfect stability.
* **Control Modes**:
  * **Mode 0 (Default)**: Use S2 (Channel 12) to select the channel (1-8) and the 6POS switch (Channel 11) to select the Band (A, B, E, F, R, L).
  * **Mode 1 (Favorite Presets)**: Use the 6POS switch (Channel 11) to quickly select between 6 custom favorite Band/Channel configurations defined in the dropdowns.
* **Pin Customization**: Change the SPI CLK, DATA, and CS pin mappings on the fly.
* **Permanent Storage**: All settings are safely saved across reboots using the ESP32 Preferences library.

---

## 🛠️ Build and Flash Instructions

This project is built using **PlatformIO**.

1. Install VS Code and the PlatformIO extension.
2. Open this project directory.
3. Connect your ESP32-S3 Super Mini to your computer via USB.
4. Build and flash the project by clicking the PlatformIO Upload button (or run `pio run -t upload` in your terminal).
5. Once booted, connect to the `ELRS_Backpack_VRX_S3` WiFi hotspot, navigate to `http://192.168.4.1/`, enter your Binding Phrase, and configure your channel mapping.
