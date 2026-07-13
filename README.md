# CRSF Direct VRX Controller for ESP32-C3 & ESP32-S3

An open-source standalone **Direct CRSF-to-VRX Controller** specifically designed for **ESP32-C3 Super Mini** and **ESP32-S3 Super Mini** boards.

This project allows your ESP32 board to be connected directly to an ELRS or Crossfire receiver via hardware UART. It parses the incoming CRSF telemetry and RC channels stream directly, mapping your radio's physical switches (e.g. 6POS Switch and S2 continuous knob) to dynamically control your video receiver (VRX) module (like RTC6715, RX5808, or Rapidfire) via standard 3-wire SPI bit-banging.

It features a beautiful, responsive web interface to configure pins, channels, select bands on the fly, and customize your channel maps.

---

## 📡 How It Works (Communication Flow)

1. Your **Radio Transmitter** sends RC channels to your **ELRS/Crossfire Receiver** on your goggles or ground station.
2. The receiver outputs standard **CRSF Serial Protocol** from its TX pad.
3. The **ESP32 (C3/S3)** is wired directly to the receiver's TX pad on a configured UART RX pin.
4. The ESP32 parses the CRSF serial packet, extracts the raw channel values (microsecond scale), and maps:
   - **Channel 11 (6POS Switch)** to change video bands.
   - **Channel 12 (S2 continuous pot)** to change channels (1-8).
5. The ESP32 issues standard SPI bit-bang commands to immediately shift the frequency of your video receiver module.

---

## 🔌 Hardware Pin Connections (ESP32 to Receiver & VRX)

### 1. Serial Connection (Receiver to ESP32)
| Function | ESP32-C3 Default Pin | ESP32-S3 Default Pin | Receiver Pad |
| :--- | :--- | :--- | :--- |
| **CRSF RX (Data In)** | **GPIO 10** | **GPIO 16** | Receiver **TX** |
| **GND** | **GND** | **GND** | GND |
| **VCC** | **5V / 3.3V** | **5V / 3.3V** | VCC |

*Note: The CRSF RX pin can be fully customized to any safe GPIO pin via the Web Configurator.*

### 2. SPI Connection (ESP32 to VRX / RTC6715)
| Function | ESP32 GPIO Pin (Default) | VRX / RTC6715 Module Pad |
| :--- | :--- | :--- |
| **CLK (Clock)** | **GPIO 4** | CLK / CH3 |
| **DATA (Data)** | **GPIO 5** | DATA / CH1 |
| **CS (Chip Select)** | **GPIO 6** | CS / CH2 / SPI_SS |
| **GND** | **GND** | GND |

---

## 💻 Web Configuration Interface

The controller hosts an open WiFi Access Point (AP) for easy on-the-fly configuration from your smartphone, PC, or FPV goggles.

* **SSID**: `CRSF_VRX_Controller`
* **Password**: None (Open)
* **IP Address**: `192.168.4.1`

### Web UI Features:
* **Live Status**: Real-time display of the currently tuned band, channel, frequency, and CRSF connection status.
* **RC Channel Bars**: Live visual monitoring of your **6POS Switch** and **S2 continuous pot** (displays microsecond values) to verify that your receiver is communicating with the ESP32.
* **Interactive VTX Grid**: A clickable 10-band x 8-channel frequency table. Click any frequency cell to manually tune the VRX! The active cell is highlighted dynamically in orange.
* **Bands Customization**:
  * **Mode 0 (Custom Band Map)**: Map each of the 6 positions of your physical 6POS switch to **any** of the 10 standard FPV bands (including both Foxeer Lowband L and Standard L!). This gives you "the ability to select the entire band" directly from your radio switches.
  * **Mode 1 (Favorite Presets)**: Use the 6POS switch to quickly select between 6 custom favorite Band/Channel configurations.
* **Pin & Baud Rate Customization**: Customize SPI pins, CRSF RX pin, and CRSF Baud rate (115200, 416700, 921600) on the fly.
* **Permanent Storage**: All settings are safely saved across reboots using the ESP32 Preferences library.

---

## 📻 FPV Frequency Grid (10 Bands)

The grid includes full support for both **Foxeer Lowband** and **Standard L-band**:

1. **Band 1 (A)**: Boscam A (`5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725`)
2. **Band 2 (B)**: Boscam B (`5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866`)
3. **Band 3 (E)**: Boscam E (`5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945`)
4. **Band 4 (F)**: Fatshark (`5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880`)
5. **Band 5 (R)**: Raceband (`5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917`)
6. **Band 6 (L Foxeer)**: Foxeer Lowband (`5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613`)
7. **Band 7 (L Std)**: Standard Lowband (`5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621`)
8. **Band 8 (U)**: Boscam U (`5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456`)
9. **Band 9 (O)**: Boscam O (`5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600`)
10. **Band 10 (H)**: Boscam H (`5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933`)

---

## 🛠️ Build and Flash Instructions

This project is built using **PlatformIO**.

1. Install VS Code and the PlatformIO extension.
2. Open this project directory.
3. Select your build target in VS Code's bottom status bar:
   - `esp32c3-supermini` (Default for ESP32-C3)
   - `esp32s3-supermini` (For ESP32-S3)
4. Connect your ESP32 board to your computer via USB.
5. Click the PlatformIO Upload button (or run `pio run -t upload` in your terminal).
6. Connect to `CRSF_VRX_Controller` WiFi hotspot, navigate to `http://192.168.4.1/`, and configure your settings.
