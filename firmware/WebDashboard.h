#pragma once
#include <Arduino.h>

/**
 * @file WebDashboard.h
 * @brief High-performance Single-Page Application stored directly in ESP8266 Flash (PROGMEM).
 * Serves real-time canvas pneumogram, vagus breathing sphere, and WebAudio bio-harmonic drone.
 */

const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Vagus Breath Engine | Contactless Spirometer</title>
  <style>
    :root {
      --bg-gradient: radial-gradient(circle at 50% 20%, #0d1b2a 0%, #050811 100%);
      --surface-glass: rgba(15, 23, 42, 0.75);
      --surface-border: rgba(56, 189, 248, 0.18);
      --accent-cyan: #38bdf8;
      --accent-teal: #2dd4bf;
      --accent-purple: #c084fc;
      --accent-coral: #fb7185;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --glow-teal: rgba(45, 212, 191, 0.35);
      --glow-cyan: rgba(56, 189, 248, 0.4);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; -webkit-font-smoothing: antialiased; }
    body { background: var(--bg-gradient); color: var(--text-main); min-height: 100vh; display: flex; flex-direction: column; overflow-x: hidden; }
    header { padding: 1.25rem 2rem; display: flex; align-items: center; justify-content: space-between; border-bottom: 1px solid var(--surface-border); backdrop-filter: blur(12px); background: rgba(8, 14, 26, 0.6); position: sticky; top: 0; z-index: 50; }
    .brand { display: flex; align-items: center; gap: 0.75rem; }
    .brand h1 { font-size: 1.25rem; font-weight: 700; background: linear-gradient(135deg, #38bdf8 0%, #2dd4bf 50%, #c084fc 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .status-badge { display: flex; align-items: center; gap: 0.5rem; padding: 0.35rem 0.85rem; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; background: rgba(45, 212, 191, 0.1); border: 1px solid rgba(45, 212, 191, 0.25); color: var(--accent-teal); }
    .status-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--accent-teal); box-shadow: 0 0 8px var(--accent-teal); animation: pulse-dot 2s infinite ease-in-out; }
    @keyframes pulse-dot { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.4; transform: scale(0.85); } }
    main { flex: 1; max-width: 1400px; margin: 0 auto; width: 100%; padding: 1.5rem; display: grid; grid-template-columns: 1fr 380px; gap: 1.5rem; }
    @media (max-width: 1024px) { main { grid-template-columns: 1fr; } }
    .card { background: var(--surface-glass); border: 1px solid var(--surface-border); border-radius: 1.25rem; padding: 1.5rem; backdrop-filter: blur(16px); box-shadow: 0 10px 30px -10px rgba(0, 0, 0, 0.5); display: flex; flex-direction: column; }
    .hero-panel { display: flex; flex-direction: column; gap: 1.5rem; }
    .zen-container { position: relative; height: 300px; display: flex; align-items: center; justify-content: center; overflow: hidden; border-radius: 1rem; background: radial-gradient(circle at center, rgba(14, 165, 233, 0.08) 0%, transparent 70%); border: 1px solid rgba(255, 255, 255, 0.05); }
    .breath-ring-outer { position: absolute; width: 250px; height: 250px; border-radius: 50%; border: 2px dashed rgba(56, 189, 248, 0.2); transition: transform 0.4s cubic-bezier(0.16, 1, 0.3, 1); }
    .breath-sphere { position: relative; width: 135px; height: 135px; border-radius: 50%; background: radial-gradient(circle at 35% 35%, #38bdf8 0%, #0d9488 60%, #042f2e 100%); box-shadow: 0 0 40px var(--glow-teal), inset 0 0 20px rgba(255, 255, 255, 0.4); display: flex; flex-direction: column; align-items: center; justify-content: center; transition: transform 0.2s ease-out, box-shadow 0.2s ease-out; transform: scale(1); }
    .breath-label { font-size: 0.85rem; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase; color: #fff; text-shadow: 0 2px 8px rgba(0,0,0,0.6); }
    .breath-sub { font-size: 0.7rem; color: rgba(255, 255, 255, 0.8); font-weight: 500; }
    .oscilloscope-card { height: 260px; display: flex; flex-direction: column; }
    .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
    .card-title { font-size: 0.95rem; font-weight: 600; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; }
    .canvas-container { flex: 1; position: relative; width: 100%; border-radius: 0.75rem; overflow: hidden; background: rgba(3, 7, 18, 0.6); border: 1px solid rgba(255, 255, 255, 0.05); }
    canvas { width: 100%; height: 100%; display: block; }
    .metrics-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 1rem; }
    .metric-box { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.06); border-radius: 1rem; padding: 1.1rem; display: flex; flex-direction: column; gap: 0.35rem; }
    .metric-label { font-size: 0.75rem; font-weight: 600; text-transform: uppercase; color: var(--text-muted); }
    .metric-value-wrap { display: flex; align-items: baseline; gap: 0.35rem; }
    .metric-value { font-size: 2rem; font-weight: 800; letter-spacing: -0.03em; color: var(--text-main); }
    .metric-unit { font-size: 0.85rem; font-weight: 500; color: var(--text-muted); }
    .coherence-bar-bg { height: 6px; width: 100%; background: rgba(255, 255, 255, 0.08); border-radius: 999px; overflow: hidden; margin-top: 0.4rem; }
    .coherence-bar-fill { height: 100%; width: 0%; border-radius: 999px; background: linear-gradient(90deg, #38bdf8, #2dd4bf); transition: width 0.3s ease-out; }
    .coach-panel { display: flex; flex-direction: column; gap: 1.25rem; }
    .coach-modes { display: flex; flex-direction: column; gap: 0.6rem; }
    .mode-btn { background: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.08); color: var(--text-main); padding: 0.85rem 1rem; border-radius: 0.75rem; cursor: pointer; display: flex; justify-content: space-between; align-items: center; transition: all 0.2s ease; text-align: left; }
    .mode-btn:hover { background: rgba(56, 189, 248, 0.1); border-color: rgba(56, 189, 248, 0.3); }
    .mode-btn.active { background: linear-gradient(135deg, rgba(56, 189, 248, 0.2), rgba(45, 212, 191, 0.2)); border-color: var(--accent-teal); box-shadow: 0 0 15px rgba(45, 212, 191, 0.15); }
    .mode-title { font-weight: 600; font-size: 0.9rem; }
    .mode-desc { font-size: 0.75rem; color: var(--text-muted); margin-top: 0.15rem; }
    .audio-toggle-btn { display: flex; align-items: center; justify-content: center; gap: 0.5rem; padding: 0.85rem; background: rgba(192, 132, 252, 0.12); border: 1px solid rgba(192, 132, 252, 0.3); color: var(--accent-purple); border-radius: 0.75rem; font-weight: 600; font-size: 0.85rem; cursor: pointer; transition: all 0.2s ease; }
    .audio-toggle-btn.playing { background: var(--accent-purple); color: #0d1b2a; box-shadow: 0 0 20px rgba(192, 132, 252, 0.4); }
  </style>
</head>
<body>
  <header>
    <div class="brand">
      <div style="font-size: 0.9rem; font-weight: 800; color: var(--accent-cyan); padding: 0.35rem 0.55rem; background: rgba(56,189,248,0.12); border: 1px solid rgba(56,189,248,0.25); border-radius: 8px;">VBE</div>
      <div>
        <h1>Vagus Breath Engine</h1>
        <div style="font-size: 0.7rem; color: var(--text-muted);">CONTACTLESS INFRARED SPIROMETER</div>
      </div>
    </div>
    <div class="status-badge" id="connStatusBadge">
      <div class="status-dot" id="statusDot"></div>
      <span id="connStatusText">Connecting...</span>
    </div>
  </header>

  <main>
    <section class="hero-panel">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Respiration State & Vagus Expansion</div>
          <div id="thermalDeltaBadge" style="font-size: 0.8rem; color: var(--accent-cyan); font-weight: 600;">ΔT: +0.00 °C</div>
        </div>
        <div class="zen-container">
          <div class="breath-ring-outer" id="breathRing"></div>
          <div class="breath-sphere" id="breathSphere">
            <span class="breath-label" id="breathStateLabel">Awaiting</span>
            <span class="breath-sub" id="breathSubLabel">Breathe towards sensor</span>
          </div>
        </div>
      </div>

      <div class="card oscilloscope-card">
        <div class="card-header">
          <div class="card-title">Real-Time Pneumogram (Thermal Breath Waveform)</div>
          <div style="font-size: 0.75rem; color: var(--text-muted);">ESP8266 25 Hz Stream</div>
        </div>
        <div class="canvas-container">
          <canvas id="pneumogramCanvas"></canvas>
        </div>
      </div>
    </section>

    <aside class="coach-panel">
      <div class="card">
        <div class="card-header">
          <div class="card-title">Live Biometric Telemetry</div>
        </div>
        <div class="metrics-grid">
          <div class="metric-box">
            <div class="metric-label">Breath Rate</div>
            <div class="metric-value-wrap">
              <span class="metric-value" id="valRPM">--</span>
              <span class="metric-unit">RPM</span>
            </div>
            <div style="font-size: 0.7rem; color: var(--text-muted);">Normal: 10 - 16</div>
          </div>

          <div class="metric-box">
            <div class="metric-label">Vagus Coherence</div>
            <div class="metric-value-wrap">
              <span class="metric-value" id="valCoherence">--</span>
              <span class="metric-unit">%</span>
            </div>
            <div class="coherence-bar-bg">
              <div class="coherence-bar-fill" id="barCoherence"></div>
            </div>
          </div>

          <div class="metric-box">
            <div class="metric-label">I : E Ratio</div>
            <div class="metric-value-wrap">
              <span class="metric-value" id="valIERatio">--</span>
            </div>
            <div style="font-size: 0.7rem; color: var(--accent-teal);">Goal: 1:2.0</div>
          </div>

          <div class="metric-box">
            <div class="metric-label">Exhale Length</div>
            <div class="metric-value-wrap">
              <span class="metric-value" id="valExhaleLen">--</span>
              <span class="metric-unit">sec</span>
            </div>
            <div style="font-size: 0.7rem; color: var(--text-muted);">Lung Power</div>
          </div>
        </div>

        <div style="margin-top: 1.25rem; padding-top: 1rem; border-top: 1px solid rgba(255,255,255,0.06); display: flex; justify-content: space-between; font-size: 0.8rem; color: var(--text-muted);">
          <span>Object: <b id="valObjTemp" style="color: var(--text-main);">--.- °C</b></span>
          <span>Ambient: <b id="valAmbTemp" style="color: var(--text-main);">--.- °C</b></span>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <div class="card-title">Vagus Training Protocols</div>
        </div>
        <div class="coach-modes">
          <button class="mode-btn active" id="btnModeFree" onclick="setCoachMode('free')">
            <div>
              <div class="mode-title">Free Flow Spirometry</div>
              <div class="mode-desc">Continuous biometric monitoring</div>
            </div>
            <span style="font-size: 0.7rem; font-weight: 700; color: var(--accent-teal); background: rgba(45,212,191,0.1); padding: 0.2rem 0.5rem; border-radius: 4px;">AUTO</span>
          </button>
          <button class="mode-btn" id="btnModeVagus" onclick="setCoachMode('vagus')">
            <div>
              <div class="mode-title">Vagus Nerve Reset (4s In / 8s Out)</div>
              <div class="mode-desc">Extended exhale parasympathetic brake</div>
            </div>
            <span style="font-size: 0.7rem; font-weight: 700; color: var(--accent-purple); background: rgba(192,132,252,0.1); padding: 0.2rem 0.5rem; border-radius: 4px;">CALM</span>
          </button>
        </div>
        <div style="margin-top: 1.25rem;">
          <button class="audio-toggle-btn" id="btnAudioDrone" onclick="toggleBioAudio()">
            <span id="audioText">Enable Bio-Harmonic Drone</span>
          </button>
        </div>
      </div>
    </aside>
  </main>

  <script>
    const canvas = document.getElementById('pneumogramCanvas');
    const ctx = canvas.getContext('2d');
    let buffer = [];
    const MAX_POINTS = 200;

    function resizeCanvas() {
      canvas.width = canvas.parentElement.clientWidth * window.devicePixelRatio;
      canvas.height = canvas.parentElement.clientHeight * window.devicePixelRatio;
      ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    }
    window.addEventListener('resize', resizeCanvas);
    resizeCanvas();

    const sphere = document.getElementById('breathSphere');
    const ring = document.getElementById('breathRing');
    const stateLabel = document.getElementById('breathStateLabel');
    const subLabel = document.getElementById('breathSubLabel');
    const deltaBadge = document.getElementById('thermalDeltaBadge');
    const valRPM = document.getElementById('valRPM');
    const valCoherence = document.getElementById('valCoherence');
    const barCoherence = document.getElementById('barCoherence');
    const valIERatio = document.getElementById('valIERatio');
    const valExhaleLen = document.getElementById('valExhaleLen');
    const valObjTemp = document.getElementById('valObjTemp');
    const valAmbTemp = document.getElementById('valAmbTemp');

    let audioCtx = null, oscPrimary = null, oscHarmonic = null, gainNode = null, isAudioPlaying = false;
    function initAudio() {
      if (audioCtx) return;
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      oscPrimary = audioCtx.createOscillator();
      oscHarmonic = audioCtx.createOscillator();
      gainNode = audioCtx.createGain();
      oscPrimary.type = 'sine'; oscPrimary.frequency.setValueAtTime(136.1, audioCtx.currentTime);
      oscHarmonic.type = 'triangle'; oscHarmonic.frequency.setValueAtTime(204.15, audioCtx.currentTime);
      const filter = audioCtx.createBiquadFilter();
      filter.type = 'lowpass'; filter.frequency.setValueAtTime(350, audioCtx.currentTime);
      gainNode.gain.setValueAtTime(0.001, audioCtx.currentTime);
      oscPrimary.connect(filter); oscHarmonic.connect(filter); filter.connect(gainNode); gainNode.connect(audioCtx.destination);
      oscPrimary.start(); oscHarmonic.start();
    }

    function toggleBioAudio() {
      initAudio();
      if (audioCtx.state === 'suspended') audioCtx.resume();
      isAudioPlaying = !isAudioPlaying;
      const btn = document.getElementById('btnAudioDrone');
      if (isAudioPlaying) {
        btn.classList.add('playing');
        document.getElementById('audioText').textContent = 'Bio-Harmonic Drone Active';
        gainNode.gain.setTargetAtTime(0.12, audioCtx.currentTime, 0.5);
      } else {
        btn.classList.remove('playing');
        document.getElementById('audioText').textContent = 'Enable Bio-Harmonic Drone';
        gainNode.gain.setTargetAtTime(0.0001, audioCtx.currentTime, 0.5);
      }
    }

    function renderWaveform() {
      const w = canvas.width / window.devicePixelRatio;
      const h = canvas.height / window.devicePixelRatio;
      ctx.clearRect(0, 0, w, h);

      const midY = h * 0.7;
      ctx.strokeStyle = 'rgba(56, 189, 248, 0.15)';
      ctx.setLineDash([4, 4]);
      ctx.beginPath(); ctx.moveTo(0, midY); ctx.lineTo(w, midY); ctx.stroke();
      ctx.setLineDash([]);

      if (buffer.length >= 2) {
        const step = w / (MAX_POINTS - 1);
        ctx.beginPath();
        buffer.forEach((pt, i) => {
          const x = i * step;
          const y = midY - (pt.delta * 70);
          if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        });
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, 'rgba(45, 212, 191, 0.35)');
        grad.addColorStop(0.7, 'rgba(56, 189, 248, 0.05)');
        grad.addColorStop(1, 'rgba(3, 7, 18, 0)');
        ctx.lineTo(w, midY); ctx.closePath();
        ctx.fillStyle = grad; ctx.fill();

        ctx.beginPath();
        buffer.forEach((pt, i) => {
          const x = i * step;
          const y = midY - (pt.delta * 70);
          if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        });
        ctx.strokeStyle = '#2dd4bf'; ctx.lineWidth = 2.5; ctx.stroke();
      }
      requestAnimationFrame(renderWaveform);
    }
    requestAnimationFrame(renderWaveform);

    function processPacket(pkt) {
      buffer.push({ delta: pkt.delta, state: pkt.state });
      if (buffer.length > MAX_POINTS) buffer.shift();

      deltaBadge.textContent = `ΔT: ${pkt.delta >= 0 ? '+' : ''}${pkt.delta.toFixed(2)} °C`;
      valObjTemp.textContent = `${pkt.obj.toFixed(1)} °C`;
      valAmbTemp.textContent = `${pkt.amb.toFixed(1)} °C`;
      valRPM.textContent = pkt.rpm > 0 ? pkt.rpm.toFixed(1) : '--';
      valCoherence.textContent = pkt.coherence > 0 ? Math.round(pkt.coherence) : '--';
      barCoherence.style.width = `${Math.min(100, Math.max(0, pkt.coherence))}%`;
      valIERatio.textContent = pkt.ie_ratio > 0 ? `1 : ${pkt.ie_ratio.toFixed(1)}` : '--';
      valExhaleLen.textContent = pkt.exhale_s > 0 ? pkt.exhale_s.toFixed(1) : '--';

      if (pkt.state === 'EXHALE') {
        stateLabel.textContent = 'EXHALING';
        subLabel.textContent = 'Warm air detected (Vagus activation)';
        const scale = 1.0 + Math.min(0.65, pkt.delta * 0.35);
        sphere.style.transform = `scale(${scale})`;
        ring.style.transform = `scale(${1.0 + scale * 0.15})`;
      } else if (pkt.state === 'INHALE') {
        stateLabel.textContent = 'INHALING';
        subLabel.textContent = 'Cooling down / filling lungs';
        sphere.style.transform = 'scale(0.85)';
        ring.style.transform = 'scale(0.9)';
      } else {
        stateLabel.textContent = 'REST / APNEA';
        subLabel.textContent = 'Breathe gently towards sensor';
        sphere.style.transform = 'scale(1.0)';
        ring.style.transform = 'scale(1.0)';
      }

      if (isAudioPlaying && audioCtx) {
        oscPrimary.frequency.setTargetAtTime(136.1 + Math.max(0, pkt.delta) * 25, audioCtx.currentTime, 0.2);
        gainNode.gain.setTargetAtTime(pkt.state === 'EXHALE' ? 0.18 : 0.08, audioCtx.currentTime, 0.4);
      }
    }

    let ws = null;
    function connectWS() {
      const host = window.location.hostname || '192.168.4.1';
      ws = new WebSocket(`ws://${host}:81/`);
      ws.onopen = () => {
        document.getElementById('connStatusText').textContent = 'ESP8266 Live';
        document.getElementById('statusDot').style.background = '#2dd4bf';
      };
      ws.onmessage = (evt) => {
        try { processPacket(JSON.parse(evt.data)); } catch (e) {}
      };
      ws.onclose = () => {
        document.getElementById('connStatusText').textContent = 'Reconnecting...';
        document.getElementById('statusDot').style.background = '#fb7185';
        setTimeout(connectWS, 2000);
      };
      ws.onerror = () => ws.close();
    }
    connectWS();

    function setCoachMode(mode) {
      document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
      if (mode === 'free') document.getElementById('btnModeFree').classList.add('active');
      if (mode === 'vagus') document.getElementById('btnModeVagus').classList.add('active');
    }
  </script>
</body>
</html>
)rawliteral";
