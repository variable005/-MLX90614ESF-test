# Circuit Heat Mapper & PCB Hotspot Locator

> Non-contact board-level thermal diagnostic tool using an **ESP8266** and an **MLX90614ESF** infrared radiometer. Pinpoints short circuits, failing LDO regulators, and overheating ICs without physical contact.

---

## Hardware Connections

The MLX90614 communicates over I2C at 3.3V logic levels.

| ESP8266 Pin | GPIO | MLX90614 Pin | Description |
|---|---|---|---|
| **3.3V** | 3.3V | **VIN / VCC** | 3.3V Power Supply |
| **GND** | GND | **GND** | Ground |
| **D1** | GPIO 5 | **SCL** | I2C Clock |
| **D2** | GPIO 4 | **SDA** | I2C Data |

---

## Operating Modes

### 1. Virtual 2D PCB Thermal Grid
- Divides your circuit board into a customizable matrix (e.g. 10x8 or 16x12 cells).
- **Auto-Dwell Capture:** Hover the sensor 5-10mm above each quadrant/chip. When the reading stabilizes, the cell locks in that value and advances.
- **Bicubic False-Color Shader:** Smoothly interpolates the discrete point measurements into a continuous 2D thermal heatmap (Deep Blue = Ambient, Green = Normal Logic, Red/White = Overheating component).

### 2. Linear Component Sweep (1D Trace)
- Drag the probe slowly across a row of components.
- The interface records the spatial thermal contour and marks the exact coordinate of the maximum thermal peak.

### 3. Spot Pyrometer & Delta Differential
- Displays live surface temperature ($T_{obj}$), ambient reference ($T_{amb}$), and excess thermal delta ($\Delta T = T_{obj} - T_{amb}$).
- Flags hotspots at a fixed threshold of $\Delta T \ge 15^{\circ}\text{C}$ or an absolute temperature of $50^{\circ}\text{C}$ (the alarm threshold is defined in `HeatMapDSP.h`).

---

## Network Configuration

- **Target Wi-Fi Network (Station mode only):**
  - **SSID:** `Pico`
  - **Password:** `12345678`
- **Hostname / mDNS:** `http://heatmapper.local` (started after the board obtains an IP address)
- **Fallback:** none. If the `Pico` network is unavailable, the board keeps retrying in the background; it does **not** create `HeatMapper-AP`.

---

## Quickstart: How to Flash & Run

### PlatformIO (recommended)
1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) or the VS Code extension.
2. From the project root run:
   ```bash
   pio run -e nodemcuv2 -t upload
   ```
3. Open Serial Monitor at **115200 baud**:
   ```bash
   pio device monitor
   ```
4. Open the printed IP address or `http://heatmapper.local` in a browser on the same Wi-Fi network.

`platformio.ini` already declares the **WebSockets** and **Adafruit MLX90614** dependencies, so no manual library installation is needed.

### Arduino IDE
1. Open `CircuitHeatMapper/CircuitHeatMapper.ino` (the folder name must match the `.ino` file name; opening `firmware/CircuitHeatMapper.ino` directly will make the IDE complain and offer to move the sketch).
2. Install these libraries via `Tools` -> `Manage Libraries...`:
   - **WebSockets** by *Markus Sattler*
   - **Adafruit MLX90614 Library** by *Adafruit* (accept the **Adafruit BusIO** dependency when prompted)
3. Select your board (e.g. `NodeMCU 1.0 (ESP-12E Module)`) and the correct serial port.
4. Click **Upload**.
5. Open Serial Monitor at **115200 baud** to see the assigned IP address, then open that IP in any browser on the same Wi-Fi.
