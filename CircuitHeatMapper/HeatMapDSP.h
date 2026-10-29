#pragma once
#include <Arduino.h>

/**
 * @file HeatMapDSP.h
 * @brief High-precision thermal signal processing for PCB circuit diagnostics.
 * Computes moving averages, ambient delta, rate of change (dT/dt), and peak tracking.
 */

struct HeatMetrics {
  float rawObj;
  float filteredObj;
  float rawAmb;
  float delta;
  float rateOfChange; // °C per second
  float peakTemp;
  bool isHotspot;
};

class HeatMapDSP {
public:
  HeatMapDSP()
    : _filteredObj(25.0f),
      _prevFiltered(25.0f),
      _peakTemp(-999.0f),
      _lastUpdateMs(0),
      _initialized(false) {}

  void init(float initialObj) {
    if (!isnan(initialObj)) {
      _filteredObj = initialObj;
      _prevFiltered = initialObj;
      _peakTemp = initialObj;
      _lastUpdateMs = millis();
      _initialized = true;
    }
  }

  HeatMetrics process(float rawObj, float rawAmb, uint32_t nowMs) {
    if (!_initialized || isnan(rawObj) || isnan(rawAmb)) {
      if (!isnan(rawObj)) init(rawObj);
      HeatMetrics m = {};
      return m;
    }

    float dt = (nowMs - _lastUpdateMs) / 1000.0f;
    if (dt <= 0.001f) dt = 0.02f;
    _lastUpdateMs = nowMs;

    // 1. Exponential Moving Average for optical noise rejection (alpha = 0.35 at ~33Hz)
    const float alpha = 0.35f;
    _filteredObj += alpha * (rawObj - _filteredObj);

    // 2. Instantaneous Rate of Change (dT/dt) in °C/sec
    float rateOfChange = (_filteredObj - _prevFiltered) / dt;
    _prevFiltered = _filteredObj;

    // 3. Excess Delta above ambient baseline
    float delta = _filteredObj - rawAmb;

    // 4. Session Peak Tracking
    if (_filteredObj > _peakTemp) {
      _peakTemp = _filteredObj;
    }

    // 5. Hotspot Warning Threshold (> 15°C above ambient or absolute > 50°C)
    bool isHotspot = (delta >= 15.0f) || (_filteredObj >= 50.0f);

    HeatMetrics m;
    m.rawObj = rawObj;
    m.filteredObj = _filteredObj;
    m.rawAmb = rawAmb;
    m.delta = delta;
    m.rateOfChange = rateOfChange;
    m.peakTemp = _peakTemp;
    m.isHotspot = isHotspot;

    return m;
  }

  void resetPeak() {
    _peakTemp = _filteredObj;
  }

private:
  float _filteredObj;
  float _prevFiltered;
  float _peakTemp;
  uint32_t _lastUpdateMs;
  bool _initialized;
};
