# Contactless Spirometer & Vagus-Nerve Bio-Feedback Engine

> A touchless respiratory laboratory and bio-feedback system using an **ESP8266** and an **MLX90614ESF** infrared radiometer. Measures breath mechanics, lung capacity, and parasympathetic vagal tone from 10–20 cm away without physical contact.

---

## Hardware Wiring

The MLX90614 communicates over standard I2C (`0x5A` default address).

| ESP8266 Pin | GPIO | MLX90614ESF Pin | Note |
|---|---|---|---|
| **3.3V** | 3.3V | **VIN / VCC** | Power supply (3.3V) |
| **GND** | GND | **GND** | Ground |
| **D1** | GPIO 5 | **SCL** | I2C Clock |
| **D2** | GPIO 4 | **SDA** | I2C Data |

*Note: Most MLX90614 breakout modules include onboard 4.7kΩ pull-up resistors on SDA/SCL.*

---

## Scientific & Engineering Principles

### 1. Contactless Thermal Radiometry of Human Breath
- Ambient indoor air: **~20°C – 24°C**
- Human expired alveolar breath: **~34°C – 36°C**
- The MLX90614 optical field of view captures the warm air packet drifting across its lens, resulting in a distinct thermal impulse $\Delta T = T_{obj} - T_{amb}$.

### 2. Embedded Digital Signal Processing (DSP) Pipeline
```
[MLX90614 Raw IR] 
       │
       ▼ (25-30 Hz Sampling)
[Dual-Pole Low-Pass Filter]  ──> Strips ADC quantization & optical noise
       │
       ▼
[Adaptive Baseline Tracker]  ──> Compensates for ambient room drift
       │
       ▼
[Derivative & Peak Extractor]──> Classifies: INHALE, EXHALE, APNEA (Hold)
       │
       ▼
[Vagus Coherence Algorithm]  ──> Analyzes I:E ratio & cadence (Target: 1:2 relaxation ratio)
       │
       ▼ (WebSocket Stream)
[Browser WebGL / Canvas HUD]
```

### 3. Vagal Tone & Respiration Coherence
Longer, controlled exhales stimulate the **Vagus nerve**, slowing heart rate and activating the parasympathetic nervous system ("rest and digest"). This engine tracks the **Inhale-to-Exhale (I:E) Ratio** and awards a real-time **Coherence Score (0-100%)**.

---

## Features

- **Zero-Contact Spirometry:** Sit in front of the sensor; no mouthpiece, mask, or chest strap needed.
- **Embedded Web Server:** ESP8266 hosts its own Wi-Fi Access Point (`Breath-Engine-AP`) and web page.
- **Real-Time WebSocket Stream:** High-framerate telemetry streamed directly to any smartphone or PC browser.
- **Interactive Visual Feedback:**
  - Real-time pneumogram / thermal breath oscilloscope.
  - Expanding Zen breathing sphere / mandala.
  - Live RPM (Breaths Per Minute), Exhale Duration, and Coherence Score.
  - Guided breathing coaches (Box Breathing 4-4-4-4, Relaxing 4-7-8, Vagus Nerve 4-8).
  - Ambient WebAudio acoustic drone synchronized with your breathing depth.

---

## Quickstart: Flashing & Running

### Option A: Using Arduino IDE
1. Open the Arduino IDE.
2. Ensure you have the **ESP8266 Board package** installed:
   - Go to `Settings` -> Additional Boards Manager URLs: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - In `Boards Manager`, search for `ESP8266` and click install.
3. Install the **WebSockets** library:
   - Go to `Sketch` -> `Include Library` -> `Manage Libraries...`
   - Search for **WebSockets** (by *Markus Sattler*) and click Install.
4. Open [`firmware/BreathEngine.ino`](file:///Users/hariom/Documents/shigoto/embedded/firmware/BreathEngine.ino).
5. Select your board (e.g. `NodeMCU 1.0 (ESP-12E Module)` or `LOLIN(WEMOS) D1 R2 & mini`).
6. Click **Upload**.

### Option B: Using VS Code + PlatformIO
If you use PlatformIO, everything is preconfigured in [`platformio.ini`](file:///Users/hariom/Documents/shigoto/embedded/platformio.ini):
```bash
pio run --target upload
```

---

## How to Connect & View

1. Power on your ESP8266.
2. On your phone or computer, look for the Wi-Fi network:
   - **SSID:** `Breath-Engine-AP`
   - **Password:** `breathewell`
3. Open your browser and navigate to:
   - **`http://192.168.4.1`**
4. Place the sensor on your desk, pointed gently towards your nose/mouth from **10 to 20 cm away**.
5. Breathe naturally. Watch the real-time pneumogram curve and expanding Zen mandala respond to your breath!

---

## Testing via Arduino Serial Plotter
You can also view the live thermal breath waveforms directly on your PC:
1. Keep the ESP8266 plugged in via USB.
2. In the Arduino IDE, go to `Tools` -> **`Serial Plotter`** (Set baud rate to `115200`).
3. You will see real-time graphical traces of `Delta`, `Baseline`, `Filtered`, and `RPM`.

