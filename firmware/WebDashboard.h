#pragma once
#include <Arduino.h>

/**
 * Circuit Heat Mapper single-page web app stored in ESP8266 Flash (PROGMEM).
 */

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Circuit Heat Mapper | PCB Thermal Diagnostics</title>
  <style>
    :root {
      --bg-dark: #070b13;
      --surface-card: rgba(15, 23, 42, 0.85);
      --border-subtle: rgba(56, 189, 248, 0.16);
      --accent-cyan: #38bdf8;
      --accent-emerald: #10b981;
      --accent-amber: #f59e0b;
      --accent-rose: #f43f5e;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", monospace, sans-serif;
      -webkit-font-smoothing: antialiased;
    }

    body {
      background: var(--bg-dark);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
    }

    header {
      padding: 1rem 1.5rem;
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-bottom: 1px solid var(--border-subtle);
      background: rgba(10, 16, 29, 0.9);
      backdrop-filter: blur(12px);
      position: sticky;
      top: 0;
      z-index: 50;
    }

    .brand-title {
      font-size: 1.1rem;
      font-weight: 700;
      letter-spacing: -0.02em;
      color: var(--text-main);
      display: flex;
      align-items: center;
      gap: 0.6rem;
    }

    .badge-hw {
      font-size: 0.7rem;
      font-weight: 700;
      padding: 0.2rem 0.5rem;
      border-radius: 4px;
      background: rgba(56, 189, 248, 0.12);
      border: 1px solid rgba(56, 189, 248, 0.3);
      color: var(--accent-cyan);
    }

    .status-pill {
      display: flex;
      align-items: center;
      gap: 0.45rem;
      font-size: 0.75rem;
      font-weight: 600;
      padding: 0.3rem 0.75rem;
      border-radius: 9999px;
      background: rgba(16, 185, 129, 0.1);
      border: 1px solid rgba(16, 185, 129, 0.25);
      color: var(--accent-emerald);
    }

    .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--accent-emerald);
      box-shadow: 0 0 8px var(--accent-emerald);
    }

    main {
      flex: 1;
      max-width: 1440px;
      margin: 0 auto;
      width: 100%;
      padding: 1.25rem;
      display: grid;
      grid-template-columns: 1fr 360px;
      gap: 1.25rem;
    }

    @media (max-width: 1024px) {
      main {
        grid-template-columns: 1fr;
      }
    }

    .card {
      background: var(--surface-card);
      border: 1px solid var(--border-subtle);
      border-radius: 0.85rem;
      padding: 1.25rem;
      display: flex;
      flex-direction: column;
      gap: 1rem;
    }

    .card-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .card-title {
      font-size: 0.85rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--text-muted);
    }

    /* Spot Telemetry Row */
    .telemetry-row {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 0.85rem;
    }

    @media (max-width: 768px) {
      .telemetry-row {
        grid-template-columns: repeat(2, 1fr);
      }
    }

    .stat-box {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 0.65rem;
      padding: 0.9rem;
      display: flex;
      flex-direction: column;
      gap: 0.25rem;
    }

    .stat-lbl {
      font-size: 0.7rem;
      font-weight: 600;
      text-transform: uppercase;
      color: var(--text-muted);
      letter-spacing: 0.05em;
    }

    .stat-val-wrap {
      display: flex;
      align-items: baseline;
      gap: 0.25rem;
    }

    .stat-val {
      font-size: 1.85rem;
      font-weight: 800;
      letter-spacing: -0.03em;
      color: var(--text-main);
      font-variant-numeric: tabular-nums;
    }

    .stat-unit {
      font-size: 0.8rem;
      font-weight: 600;
      color: var(--text-muted);
    }

    /* Heatmap Canvas Container */
    .canvas-wrapper {
      position: relative;
      width: 100%;
      height: 420px;
      border-radius: 0.65rem;
      overflow: hidden;
      background: #030712;
      border: 1px solid rgba(255, 255, 255, 0.08);
      display: flex;
      align-items: center;
      justify-content: center;
    }

    canvas#heatmapCanvas {
      width: 100%;
      height: 100%;
      display: block;
      cursor: crosshair;
    }

    .canvas-overlay-hud {
      position: absolute;
      top: 0.75rem;
      left: 0.75rem;
      background: rgba(3, 7, 18, 0.85);
      border: 1px solid var(--border-subtle);
      border-radius: 6px;
      padding: 0.4rem 0.65rem;
      font-size: 0.75rem;
      font-weight: 600;
      display: flex;
      gap: 0.85rem;
      backdrop-filter: blur(8px);
    }

    /* Temperature Scale Legend Bar */
    .scale-bar-container {
      display: flex;
      align-items: center;
      gap: 0.75rem;
      font-size: 0.75rem;
      font-weight: 600;
      color: var(--text-muted);
    }

    .scale-gradient {
      flex: 1;
      height: 10px;
      border-radius: 9999px;
      background: linear-gradient(90deg, 
        #05081c 0%, 
        #1e3a8a 20%, 
        #0284c7 40%, 
        #10b981 60%, 
        #f59e0b 80%, 
        #ef4444 92%, 
        #ffffff 100%
      );
      border: 1px solid rgba(255, 255, 255, 0.1);
    }

    /* Controls Panel */
    .control-group {
      display: flex;
      flex-direction: column;
      gap: 0.65rem;
    }

    .btn {
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.1);
      color: var(--text-main);
      padding: 0.7rem 0.9rem;
      border-radius: 0.5rem;
      font-size: 0.8rem;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: space-between;
      transition: all 0.2s ease;
    }

    .btn:hover {
      background: rgba(56, 189, 248, 0.1);
      border-color: rgba(56, 189, 248, 0.3);
    }

    .btn-primary {
      background: rgba(56, 189, 248, 0.15);
      border-color: var(--accent-cyan);
      color: #fff;
    }

    .btn-primary:hover {
      background: rgba(56, 189, 248, 0.25);
    }

    .btn.active {
      background: rgba(16, 185, 129, 0.2);
      border-color: var(--accent-emerald);
      color: #fff;
    }

    /* Linear Sweep Chart */
    .scope-box {
      height: 160px;
      position: relative;
      border-radius: 0.5rem;
      overflow: hidden;
      background: #030712;
      border: 1px solid rgba(255, 255, 255, 0.06);
    }

    canvas#scopeCanvas {
      width: 100%;
      height: 100%;
      display: block;
    }
  </style>
</head>
<body>

  <header>
    <div class="brand-title">
      <span>Circuit Heat Mapper</span>
      <span class="badge-hw">ESP8266 + MLX90614</span>
    </div>
    <div style="display: flex; align-items: center; gap: 0.6rem;">
      <input type="text" id="targetHostInput" placeholder="heatmapper.local or IP" style="background: rgba(255,255,255,0.06); border: 1px solid var(--border-subtle); color: #fff; padding: 0.35rem 0.65rem; border-radius: 6px; font-size: 0.75rem; width: 170px;" value="heatmapper.local">
      <button onclick="reconnectToHost()" style="background: rgba(56,189,248,0.2); border: 1px solid var(--accent-cyan); color: #fff; padding: 0.35rem 0.65rem; border-radius: 6px; font-size: 0.75rem; font-weight: 600; cursor: pointer;">Connect</button>
      <div class="status-pill" id="connPill">
        <div class="dot" id="statusDot"></div>
        <span id="connLabel">Connecting...</span>
      </div>
    </div>
  </header>

  <main>
    <section style="display: flex; flex-direction: column; gap: 1.25rem;">
      <!-- Spot Metrics Row -->
      <div class="telemetry-row">
        <div class="stat-box">
          <span class="stat-lbl">Component Temp</span>
          <div class="stat-val-wrap">
            <span class="stat-val" id="valObj">--.-</span>
            <span class="stat-unit">°C</span>
          </div>
        </div>

        <div class="stat-box">
          <span class="stat-lbl">Ambient Reference</span>
          <div class="stat-val-wrap">
            <span class="stat-val" id="valAmb">--.-</span>
            <span class="stat-unit">°C</span>
          </div>
        </div>

        <div class="stat-box">
          <span class="stat-lbl">Excess Delta (ΔT)</span>
          <div class="stat-val-wrap">
            <span class="stat-val" id="valDelta" style="color: var(--accent-cyan);">--.-</span>
            <span class="stat-unit">°C</span>
          </div>
        </div>

        <div class="stat-box">
          <span class="stat-lbl">Maximum Hotspot</span>
          <div class="stat-val-wrap">
            <span class="stat-val" id="valMaxHot" style="color: var(--accent-rose);">--.-</span>
            <span class="stat-unit">°C</span>
          </div>
        </div>
      </div>

      <!-- 2D PCB Thermal Heatmap Canvas -->
      <div class="card">
        <div class="card-head">
          <span class="card-title">2D PCB Thermal Matrix (Interpolated Heatmap)</span>
          <span style="font-size: 0.75rem; color: var(--text-muted);" id="gridDimLabel">Grid: 10 x 8 (80 Points)</span>
        </div>

        <div class="canvas-wrapper">
          <canvas id="heatmapCanvas"></canvas>
          <div class="canvas-overlay-hud">
            <span>Target: <b id="hudCursorCoord">R1:C1</b></span>
            <span>Cell Temp: <b id="hudCellTemp">--.- °C</b></span>
            <span>Hotspot Peak: <b id="hudMaxPeak">--.- °C</b></span>
          </div>
        </div>

        <div class="scale-bar-container">
          <span id="scaleMin">20°C (Cold Ground)</span>
          <div class="scale-gradient"></div>
          <span id="scaleMax">70°C+ (Overheating)</span>
        </div>
      </div>

      <!-- Linear Sweep Oscilloscope -->
      <div class="card">
        <div class="card-head">
          <span class="card-title">Linear Component Sweep Profile (1D Trace)</span>
          <span style="font-size: 0.75rem; color: var(--text-muted);">Sampling: 33 Hz</span>
        </div>
        <div class="scope-box">
          <canvas id="scopeCanvas"></canvas>
        </div>
      </div>
    </section>

    <!-- Side Control Panel -->
    <aside style="display: flex; flex-direction: column; gap: 1.25rem;">
      <div class="card">
        <div class="card-head">
          <span class="card-title">Grid Scan Controls</span>
        </div>

        <div class="control-group">
          <button class="btn btn-primary" id="btnCaptureCell" onclick="captureCurrentCell()">
            <span>Capture Active Cell</span>
            <span style="font-size: 0.7rem; opacity: 0.7;">[SPACEBAR]</span>
          </button>

          <button class="btn" id="btnAutoDwell" onclick="toggleAutoDwell()">
            <span>Auto-Dwell Scanner</span>
            <span id="autoDwellTag" style="font-size: 0.7rem; color: var(--text-muted);">OFF</span>
          </button>

          <button class="btn" onclick="nextCell()">
            <span>Next Cell Coordinate</span>
            <span>→</span>
          </button>

          <button class="btn" onclick="prevCell()">
            <span>Previous Cell</span>
            <span>←</span>
          </button>

          <button class="btn" onclick="resetGrid()">
            <span>Clear Heatmap Matrix</span>
            <span>↺</span>
          </button>
        </div>
      </div>

      <div class="card">
        <div class="card-head">
          <span class="card-title">Audio & Alarm Threshold</span>
        </div>

        <div class="control-group">
          <button class="btn" id="btnAudioTone" onclick="toggleAudioTone()">
            <span>Thermal Pyrometer Audio</span>
            <span id="audioTag" style="font-size: 0.7rem; color: var(--text-muted);">OFF</span>
          </button>

          <button class="btn" onclick="exportCSV()">
            <span>Export Matrix to CSV</span>
            <span>↓</span>
          </button>
        </div>

        <div style="font-size: 0.75rem; color: var(--text-muted); line-height: 1.4; margin-top: 0.5rem;">
          <b>Probing Technique:</b> Hold the sensor 5mm–10mm above PCB traces for sharp spatial resolution. Avoid touching live electrical contacts.
        </div>
      </div>
    </aside>
  </main>

  <script>
    // --- Grid Configuration ---
    const COLS = 10;
    const ROWS = 8;
    let gridData = Array.from({ length: ROWS }, () => Array(COLS).fill(null));
    let activeRow = 0;
    let activeCol = 0;
    let currentObjTemp = 24.0;
    let currentAmbTemp = 23.5;
    let currentDelta = 0.5;
    let maxDetectedHotspot = 24.0;

    // Canvases
    const hCanvas = document.getElementById('heatmapCanvas');
    const hCtx = hCanvas.getContext('2d');
    const sCanvas = document.getElementById('scopeCanvas');
    const sCtx = sCanvas.getContext('2d');
    let sweepBuffer = [];

    function resizeCanvases() {
      hCanvas.width = hCanvas.parentElement.clientWidth * window.devicePixelRatio;
      hCanvas.height = hCanvas.parentElement.clientHeight * window.devicePixelRatio;
      hCtx.scale(window.devicePixelRatio, window.devicePixelRatio);

      sCanvas.width = sCanvas.parentElement.clientWidth * window.devicePixelRatio;
      sCanvas.height = sCanvas.parentElement.clientHeight * window.devicePixelRatio;
      sCtx.scale(window.devicePixelRatio, window.devicePixelRatio);
    }
    window.addEventListener('resize', resizeCanvases);
    resizeCanvases();

    // DOM Elements
    const elObj = document.getElementById('valObj');
    const elAmb = document.getElementById('valAmb');
    const elDelta = document.getElementById('valDelta');
    const elMaxHot = document.getElementById('valMaxHot');
    const hudCoord = document.getElementById('hudCursorCoord');
    const hudCell = document.getElementById('hudCellTemp');
    const hudMax = document.getElementById('hudMaxPeak');
    const connLabel = document.getElementById('connLabel');
    const statusDot = document.getElementById('statusDot');

    // Thermal Color Palette (Ironbow / Turbo Shader)
    function getThermalColor(temp, minT = 22, maxT = 65) {
      if (temp === null || isNaN(temp)) return 'rgba(15, 23, 42, 0.4)';
      let norm = Math.min(1, Math.max(0, (temp - minT) / (maxT - minT)));

      // Color stops: Deep Navy (0.0) -> Blue (0.2) -> Cyan (0.4) -> Green (0.6) -> Yellow (0.8) -> Red (0.95) -> White (1.0)
      let r, g, b;
      if (norm < 0.2) {
        let t = norm / 0.2;
        r = Math.round(5 + t * 25);
        g = Math.round(8 + t * 50);
        b = Math.round(28 + t * 110);
      } else if (norm < 0.4) {
        let t = (norm - 0.2) / 0.2;
        r = Math.round(30 - t * 28);
        g = Math.round(58 + t * 74);
        b = Math.round(138 + t * 61);
      } else if (norm < 0.6) {
        let t = (norm - 0.4) / 0.2;
        r = Math.round(2 + t * 14);
        g = Math.round(132 + t * 53);
        b = Math.round(199 - t * 70);
      } else if (norm < 0.8) {
        let t = (norm - 0.6) / 0.2;
        r = Math.round(16 + t * 229);
        g = Math.round(185 - t * 27);
        b = Math.round(129 - t * 118);
      } else if (norm < 0.95) {
        let t = (norm - 0.8) / 0.15;
        r = Math.round(245 - t * 6);
        g = Math.round(158 - t * 90);
        b = Math.round(11 + t * 57);
      } else {
        let t = (norm - 0.95) / 0.05;
        r = Math.round(239 + t * 16);
        g = Math.round(68 + t * 187);
        b = Math.round(68 + t * 187);
      }
      return `rgb(${r},${g},${b})`;
    }

    // Render 2D Heatmap Grid
    function drawHeatmap() {
      const w = hCanvas.width / window.devicePixelRatio;
      const h = hCanvas.height / window.devicePixelRatio;
      hCtx.clearRect(0, 0, w, h);

      const cellW = w / COLS;
      const cellH = h / ROWS;

      // Draw Cells
      for (let r = 0; r < ROWS; r++) {
        for (let c = 0; c < COLS; c++) {
          const val = gridData[r][c];
          hCtx.fillStyle = getThermalColor(val);
          hCtx.fillRect(c * cellW, r * cellH, cellW, cellH);

          // Grid Borders
          hCtx.strokeStyle = 'rgba(255, 255, 255, 0.07)';
          hCtx.lineWidth = 1;
          hCtx.strokeRect(c * cellW, r * cellH, cellW, cellH);

          // Text labels for captured values
          if (val !== null) {
            hCtx.fillStyle = val > 45 ? '#ffffff' : 'rgba(255,255,255,0.75)';
            hCtx.font = '10px monospace';
            hCtx.textAlign = 'center';
            hCtx.textBaseline = 'middle';
            hCtx.fillText(`${val.toFixed(1)}°`, c * cellW + cellW / 2, r * cellH + cellH / 2);
          }
        }
      }

      // Draw Active Target Cursor Box
      const activeX = activeCol * cellW;
      const activeY = activeRow * cellH;
      hCtx.strokeStyle = '#38bdf8';
      hCtx.lineWidth = 2.5;
      hCtx.strokeRect(activeX + 2, activeY + 2, cellW - 4, cellH - 4);

      // Pulse ring inside active cell
      hCtx.fillStyle = 'rgba(56, 189, 248, 0.15)';
      hCtx.fillRect(activeX + 2, activeY + 2, cellW - 4, cellH - 4);
    }

    // Render 1D Sweep Scope
    function drawScope() {
      const w = sCanvas.width / window.devicePixelRatio;
      const h = sCanvas.height / window.devicePixelRatio;
      sCtx.clearRect(0, 0, w, h);

      // Grid
      sCtx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
      sCtx.lineWidth = 1;
      for (let y = 0; y < h; y += 25) {
        sCtx.beginPath();
        sCtx.moveTo(0, y);
        sCtx.lineTo(w, y);
        sCtx.stroke();
      }

      if (sweepBuffer.length < 2) return;

      const step = w / (sweepBuffer.length - 1);
      const minTemp = 20;
      const maxTemp = 75;

      sCtx.beginPath();
      sweepBuffer.forEach((pt, i) => {
        const x = i * step;
        const norm = Math.min(1, Math.max(0, (pt - minTemp) / (maxTemp - minTemp)));
        const y = h - (norm * (h - 20)) - 10;
        if (i === 0) sCtx.moveTo(x, y);
        else sCtx.lineTo(x, y);
      });
      sCtx.strokeStyle = '#38bdf8';
      sCtx.lineWidth = 2;
      sCtx.stroke();
    }

    function animate() {
      drawHeatmap();
      drawScope();
      requestAnimationFrame(animate);
    }
    requestAnimationFrame(animate);

    // Audio Pyrometer (Pitch proportional to temperature)
    let audioCtx = null;
    let osc = null;
    let gainNode = null;
    let isAudioOn = false;

    function initAudio() {
      if (audioCtx) return;
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      osc = audioCtx.createOscillator();
      gainNode = audioCtx.createGain();
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(220, audioCtx.currentTime);
      gainNode.gain.setValueAtTime(0.001, audioCtx.currentTime);
      osc.connect(gainNode);
      gainNode.connect(audioCtx.destination);
      osc.start();
    }

    function toggleAudioTone() {
      initAudio();
      if (audioCtx.state === 'suspended') audioCtx.resume();
      isAudioOn = !isAudioOn;
      const tag = document.getElementById('audioTag');
      if (isAudioOn) {
        tag.textContent = 'ON';
        tag.style.color = '#10b981';
        gainNode.gain.setTargetAtTime(0.06, audioCtx.currentTime, 0.2);
      } else {
        tag.textContent = 'OFF';
        tag.style.color = 'var(--text-muted)';
        gainNode.gain.setTargetAtTime(0.0001, audioCtx.currentTime, 0.2);
      }
    }

    function modulateAudio(temp) {
      if (!isAudioOn || !audioCtx) return;
      // Frequency ramps from 220Hz (22°C) to 1200Hz (70°C+)
      const freq = 220 + Math.max(0, temp - 22) * 22;
      osc.frequency.setTargetAtTime(freq, audioCtx.currentTime, 0.1);
    }

    // Packet Processor
    let sensorReady = true;

    function setConnectionState(label, color) {
      connLabel.textContent = label;
      statusDot.style.background = color;
    }

    function processPacket(pkt) {
      if (pkt.online === false) {
        sensorReady = false;
        if (ws && ws.readyState === WebSocket.OPEN) {
          setConnectionState('ESP8266 Live (No Sensor)', '#f59e0b');
        }
        elObj.textContent = '--.-';
        elAmb.textContent = '--.-';
        elDelta.textContent = '--.-';
        hudCell.textContent = 'SENSOR OFFLINE';
        return;
      }

      if (typeof pkt.t_obj !== 'number' || typeof pkt.t_amb !== 'number') return;

      if (!sensorReady) {
        sensorReady = true;
        if (ws && ws.readyState === WebSocket.OPEN) {
          setConnectionState('ESP8266 Live', '#10b981');
        }
      }

      currentObjTemp = pkt.t_obj;
      currentAmbTemp = pkt.t_amb;
      currentDelta = pkt.delta;

      elObj.textContent = currentObjTemp.toFixed(1);
      elAmb.textContent = currentAmbTemp.toFixed(1);
      elDelta.textContent = `${currentDelta >= 0 ? '+' : ''}${currentDelta.toFixed(1)}`;

      if (currentObjTemp > maxDetectedHotspot) {
        maxDetectedHotspot = currentObjTemp;
        elMaxHot.textContent = maxDetectedHotspot.toFixed(1);
        hudMax.textContent = `${maxDetectedHotspot.toFixed(1)} °C`;
      }

      hudCoord.textContent = `R${activeRow + 1}:C${activeCol + 1}`;
      hudCell.textContent = gridData[activeRow][activeCol] !== null 
        ? `${gridData[activeRow][activeCol].toFixed(1)} °C` 
        : 'EMPTY';

      sweepBuffer.push(currentObjTemp);
      if (sweepBuffer.length > 120) sweepBuffer.shift();

      modulateAudio(currentObjTemp);
      checkAutoDwell(currentObjTemp);
    }

    // Grid Navigation & Capture
    function captureCurrentCell() {
      gridData[activeRow][activeCol] = currentObjTemp;
      nextCell();
    }

    function nextCell() {
      activeCol++;
      if (activeCol >= COLS) {
        activeCol = 0;
        activeRow++;
        if (activeRow >= ROWS) activeRow = 0;
      }
    }

    function prevCell() {
      activeCol--;
      if (activeCol < 0) {
        activeCol = COLS - 1;
        activeRow--;
        if (activeRow < 0) activeRow = ROWS - 1;
      }
    }

    function resetGrid() {
      gridData = Array.from({ length: ROWS }, () => Array(COLS).fill(null));
      maxDetectedHotspot = currentObjTemp;
      elMaxHot.textContent = '--.-';
      hudMax.textContent = '--.- °C';
      activeRow = 0;
      activeCol = 0;
    }

    // Auto Dwell Logic
    let isAutoDwell = false;
    let dwellStart = 0;
    let lastDwellTemp = 0;
    let dwellArmed = true;

    function toggleAutoDwell() {
      isAutoDwell = !isAutoDwell;
      if (isAutoDwell) {
        dwellStart = 0;
        lastDwellTemp = currentObjTemp;
        dwellArmed = true;
      }
      const tag = document.getElementById('autoDwellTag');
      tag.textContent = isAutoDwell ? 'ACTIVE' : 'OFF';
      tag.style.color = isAutoDwell ? '#10b981' : 'var(--text-muted)';
    }

    function checkAutoDwell(temp) {
      if (!isAutoDwell) return;
      const now = Date.now();

      // After a capture, wait for the probe to move before arming the next cell.
      if (!dwellArmed) {
        if (Math.abs(temp - lastDwellTemp) > 0.5) {
          dwellArmed = true;
          dwellStart = now;
          lastDwellTemp = temp;
        }
        return;
      }

      if (Math.abs(temp - lastDwellTemp) < 0.25) {
        if (dwellStart === 0) dwellStart = now;
        else if (now - dwellStart > 500) {
          // Locked in!
          captureCurrentCell();
          dwellArmed = false;
          dwellStart = 0;
          lastDwellTemp = temp;
        }
      } else {
        dwellStart = now;
        lastDwellTemp = temp;
      }
    }

    // Keyboard Shortcuts (Spacebar to capture)
    window.addEventListener('keydown', (e) => {
      const target = e.target;
      const tag = (target && target.tagName) || '';
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || (target && target.isContentEditable)) {
        return; // Don't hijack typing in the host input
      }
      if (e.code === 'Space') {
        e.preventDefault();
        captureCurrentCell();
      } else if (e.code === 'ArrowRight') {
        nextCell();
      } else if (e.code === 'ArrowLeft') {
        prevCell();
      }
    });

    // CSV Export
    function exportCSV() {
      let csv = 'Row,Col,Temp_C\n';
      for (let r = 0; r < ROWS; r++) {
        for (let c = 0; c < COLS; c++) {
          csv += `${r + 1},${c + 1},${gridData[r][c] !== null ? gridData[r][c].toFixed(2) : ''}\n`;
        }
      }
      const blob = new Blob([csv], { type: 'text/csv' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `pcb_thermal_scan_${Date.now()}.csv`;
      a.click();
    }

    // WebSocket Client
    let ws = null;
    let wsHost = window.location.hostname || 'heatmapper.local';
    let reconnectTimer = null;
    let connectionId = 0;

    const hostInput = document.getElementById('targetHostInput');
    if (hostInput) hostInput.value = wsHost;

    function parseHost(value) {
      let host = value.trim().replace(/^[a-z]+:\/\//i, '');
      host = host.split('/')[0].replace(/:81$/i, '');
      return host || 'heatmapper.local';
    }

    function reconnectToHost() {
      if (!hostInput) return;
      wsHost = parseHost(hostInput.value);
      hostInput.value = wsHost;
      connectWS();
    }

    function scheduleReconnect(id) {
      if (reconnectTimer) clearTimeout(reconnectTimer);
      reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        if (id === connectionId && (!ws || ws.readyState !== WebSocket.OPEN)) {
          connectWS();
        }
      }, 3500);
    }

    function connectWS() {
      const myId = ++connectionId;

      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
      }
      if (ws) {
        ws.onopen = ws.onmessage = ws.onclose = ws.onerror = null;
        try { ws.close(); } catch (e) {}
        ws = null;
      }

      setConnectionState('Connecting...', '#38bdf8');

      let socket;
      try {
        socket = new WebSocket(`ws://${wsHost}:81/`);
      } catch (err) {
        setConnectionState('Demo Mode (Offline)', '#f59e0b');
        scheduleReconnect(myId);
        return;
      }
      ws = socket;

      socket.onopen = () => {
        if (myId !== connectionId) return;
        setConnectionState('ESP8266 Live', '#10b981');
      };
      socket.onmessage = (evt) => {
        if (myId !== connectionId) return;
        try {
          processPacket(JSON.parse(evt.data));
        } catch (e) {}
      };
      socket.onerror = () => {
        if (myId === connectionId) {
          try { socket.close(); } catch (e) {}
        }
      };
      socket.onclose = () => {
        if (myId !== connectionId) return;
        ws = null;
        setConnectionState('Demo Mode (Offline)', '#f59e0b');
        scheduleReconnect(myId);
      };
    }
    connectWS();

    // Fallback simulation for offline testing
    let simT = 0;
    setInterval(() => {
      if (ws && ws.readyState === WebSocket.OPEN) return;
      simT += 0.05;
      const base = 24.2;
      const noise = (Math.random() - 0.5) * 0.2;
      const simulatedHotspot = (activeRow === 2 && activeCol === 3) ? 48.5 : 0;
      processPacket({
        online: true,
        t_obj: base + simulatedHotspot + noise + Math.sin(simT * 0.8) * 2,
        t_amb: 23.8,
        delta: Math.max(0, simulatedHotspot + noise + Math.sin(simT * 0.8) * 2)
      });
    }, 50);
  </script>
</body>
</html>

)rawliteral";
