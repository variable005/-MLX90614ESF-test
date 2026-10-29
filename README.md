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
- Flags components that exceed a user-configurable thermal threshold (e.g. $> 45^{\circ}\text{C}$ or $\Delta T > 15^{\circ}\text{C}$).

---

## Network Configuration

- **Target Wi-Fi Network:**
  - **SSID:** `Pico`
  - **Password:** `12345678`
- **Fallback Access Point (if router is offline):**
  - **SSID:** `HeatMapper-AP`
  - **IP:** `http://192.168.4.1`

---

## Quickstart: How to Flash & Run

### Arduino IDE
1. Open [`firmware/CircuitHeatMapper.ino`](file:///Users/hariom/Documents/shigoto/embedded/firmware/CircuitHeatMapper.ino).
2. Install the **WebSockets** library by *Markus Sattler* (`Sketch` -> `Include Library` -> `Manage Libraries...`).
3. Select your board (e.g. `NodeMCU 1.0 (ESP-12E Module)`) and port `/dev/cu.usbserial-A5069RR4`.
4. Click **Upload**.
5. Open Serial Monitor at **115200 baud** to see the assigned IP address, then open that IP in any browser on the same Wi-Fi.
