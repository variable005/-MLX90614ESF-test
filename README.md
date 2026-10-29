# Circuit Heat Mapper & PCB Hotspot Locator

> Non-contact board-level thermal diagnostic tool using an **ESP8266**, an **MLX90614ESF** infrared radiometer, and an **SSD1306 OLED**. Pinpoints short circuits, failing LDO regulators, and overheating ICs without physical contact.

---

## Hardware Connections

The MLX90614 and the OLED are both I2C devices. They can share one SDA/SCL pair or use two separate pairs. The firmware scans four common pin pairs, tracks the display bus and sensor bus independently, and switches the I2C peripheral between them:

| Bus | SDA | SCL |
|---|---|---|
| A | D2 (GPIO 4) | D1 (GPIO 5) |
| B | D1 (GPIO 5) | D2 (GPIO 4) |
| C | D6 (GPIO 12) | D5 (GPIO 14) |
| D | D5 (GPIO 14) | D6 (GPIO 12) |

### Recommended standard wiring (D1/D2)

| ESP8266 Pin | GPIO | OLED | MLX90614 | Description |
|---|---|---|---|---|
| **3.3V** | 3.3V | **VCC** | **VDD** | 3.3V supply (use 5V VIN for regulator breakouts; see notes) |
| **GND** | GND | **GND** | **VSS / GND** | Common ground |
| **D1** | GPIO 5 | **SCL** | **SCL** | I2C clock |
| **D2** | GPIO 4 | **SDA** | **SDA** | I2C data |

### Wiring currently used on this board (separate buses)

The OLED is on **D6 (SDA) / D5 (SCL)** and the MLX90614 is on **D1 (SCL) / D2 (SDA)**:

| ESP8266 Pin | GPIO | OLED | MLX90614 |
|---|---|---|---|
| **3.3V** | 3.3V | **VCC** | - |
| **VIN (5V)** | 5V | - | **VCC** (HW-691 regulator input) |
| **GND** | GND | **GND** | **VSS / GND** |
| **D6** | GPIO 12 | **SDA** | - |
| **D5** | GPIO 14 | **SCL** | - |
| **D2** | GPIO 4 | - | **SDA** |
| **D1** | GPIO 5 | - | **SCL** |

Both devices can also share a single pair (for example D1/D2). The firmware detects which layout is connected and switches between the two buses automatically. If your MLX breakout is explicitly marked 3.3V-only, power it from 3.3V instead of VIN.

### MLX90614 notes

- **Bare TO-39 pinout** (official Melexis datasheet, looking at the pin side): **1 = SCL, 2 = SDA, 3 = VDD, 4 = VSS/GND**. Many hobby diagrams show a different order, so verify against the datasheet or your breakout board's silkscreen.
- **Supply voltage depends on the part suffix:**
  - `MLX90614ESF-Bxx` → **3.3V** (safe with ESP8266 logic)
  - `MLX90614ESF-Axx` → **5V** (needs level shifting for the ESP8266's 3.3V I2C lines)
- **Breakout modules (HW-691, GY-906, etc.):** most carry a small 3.3V regulator (often marked `662K`) plus I2C level shifting. Power the module's **VCC from 5V (VIN/USB)** if it has a regulator; feeding 3.3V into a regulator with too much dropout can leave the MLX itself undervolted and silent. The module's I2C pins remain safe for the ESP8266's 3.3V logic. If your module is explicitly marked 3.3V-only, use 3.3V.
- A bare sensor needs **4.7k pull-ups from SDA and SCL to 3.3V**. Most OLED modules already include pull-ups; if both devices share the bus, that is usually enough.
- The MLX90614 must be in **SMBus/I2C mode** (the factory default), not PWM mode.

### OLED notes

- Default: **SSD1306, 128x64, address 0x3C or 0x3D**.
- For a 128x32 panel, build with `-DOLED_HEIGHT=32` or change `OLED_HEIGHT` in `firmware/DisplayManager.h`.
- If the OLED is a **SH1106** (some 1.3" modules), it is not an SSD1306 and needs a different driver.

---

## OLED Display

When the OLED is detected, it shows:

- Object temperature (`T_obj`) in large digits
- Ambient temperature and `ΔT = T_obj - T_amb`
- Session peak temperature
- `OK` / inverted `HOT!` status bar
- Wi-Fi IP address in the top-right corner

If the MLX90614 is missing, the OLED shows `MLX90614 / NOT DETECTED`, the active bus, the OLED address, the IP, and a wiring reminder. The firmware keeps retrying the sensor every 2.5 seconds, so it will recover automatically once the wiring is fixed.

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

`platformio.ini` already declares the **WebSockets**, **Adafruit MLX90614**, **Adafruit SSD1306**, and **Adafruit GFX** dependencies, so no manual library installation is needed.

### Arduino IDE
1. Open `CircuitHeatMapper/CircuitHeatMapper.ino` (the folder name must match the `.ino` file name; opening `firmware/CircuitHeatMapper.ino` directly will make the IDE complain and offer to move the sketch).
2. Install these libraries via `Tools` -> `Manage Libraries...`:
   - **WebSockets** by *Markus Sattler*
   - **Adafruit MLX90614 Library** by *Adafruit* (accept the **Adafruit BusIO** dependency when prompted)
   - **Adafruit SSD1306** by *Adafruit*
   - **Adafruit GFX Library** by *Adafruit*
3. Select your board (e.g. `NodeMCU 1.0 (ESP-12E Module)`) and the correct serial port.
4. Click **Upload**.
5. Open Serial Monitor at **115200 baud** to see the assigned IP address, then open that IP in any browser on the same Wi-Fi.

---

## Troubleshooting

### OLED works, but the MLX90614 is not detected

The serial log will show something like:

```
[I2C] D2/D1 (SDA=4, SCL=5): 0 device(s)
[I2C] D6/D5 (SDA=12, SCL=14): 1 device(s) 0x3C [OLED]
[I2C] Sensor bus D2/D1, display bus D6/D5 (separate)
[MLX] Probing 0x5A on D2/D1... FAILED (no valid temperature).
```

That means the I2C bus itself is healthy (the OLED ACKs), but the MLX90614 is not answering at address `0x5A`. Check, in this order:

1. **Power the module correctly.** Measure the voltage directly between the MLX module's **VCC and GND pins** (not at the ESP8266 board). It must be in the MLX's valid range. For a regulator/level-shifter breakout such as the HW-691, try **5V on VCC** (NodeMCU `VIN`); 3.3V into the regulator can brown out the sensor. For a bare `-Bxx` part, use 3.3V.
2. **SDA and SCL are not swapped.** For a bare TO-39: pin 1 = SCL, pin 2 = SDA, pin 3 = VDD, pin 4 = GND. For a breakout board, follow its labels.
3. **The sensor is on a bus the firmware scans.** The firmware scans D2/D1, D1/D2, D6/D5, and D5/D6, so the MLX can share the OLED bus or use its own pair. Check the `[I2C]` lines in the serial log to see which pair was scanned.
4. **Pull-ups.** A bare MLX needs 4.7k pull-ups from SDA and SCL to 3.3V. Breakout modules usually include them. With the bus idle, SDA and SCL should both sit near 3.3V.
5. **Supply variant.** `-Bxx` is 3.3V; `-Axx` is 5V. A 5V part powered from 3.3V may not start, and a 5V part's I2C thresholds may not be compatible with the ESP8266's 3.3V logic without level shifting.
6. **PWM mode.** A used/reconfigured sensor may be in PWM mode instead of SMBus mode; it will not ACK I2C.
7. **Header/solder joints.** Wiggle each Dupont wire and check continuity from the ESP8266 pin to the module pin. Unsoldered or cold-soldered headers are a common cause of a completely silent module.

The firmware retries every 2.5 seconds, so no reboot is needed after fixing the wiring.

### OLED is blank

- Confirm the OLED ACKs at `0x3C` or `0x3D` in the serial scan.
- Most modules are 128x64 SSD1306. If yours is 128x32, set `OLED_HEIGHT=32`.
- If it is an SH1106 (1.3" modules), it needs an SH1106 driver, not SSD1306.
- Check that the OLED is on the same bus that the firmware selected.

### Diagnostic scanner

`I2C_Scanner/I2C_Scanner.ino` (also copied to `firmware/tools/I2C_Scanner.ino`) is a standalone Arduino sketch that scans all four pin pairs at 10 kHz, 50 kHz, and 100 kHz, reports every ACKing address, and attempts a direct MLX90614 register read. Flash it with the Arduino IDE if you want to verify the wiring independently of the main firmware.
