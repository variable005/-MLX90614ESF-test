#pragma once
#include <Arduino.h>

/**
 * @file BreathDSP.h
 * @brief Real-Time Digital Signal Processing for Non-Contact Infrared Breath Analysis
 * Extracts Respiration Rate (RPM), Exhale Duration, Inhale:Exhale Ratio, and Vagus Coherence.
 */

enum BreathState {
  BREATH_IDLE = 0,
  BREATH_EXHALE = 1,
  BREATH_INHALE = 2
};

struct BreathMetrics {
  float rawTemp;
  float filteredTemp;
  float ambientTemp;
  float baselineTemp;
  float deltaTemp;
  float derivative;
  BreathState state;
  const char* stateStr;
  float rpm;
  float exhaleSeconds;
  float inhaleSeconds;
  float ieRatio;
  float coherenceScore; // 0 - 100%
};

class BreathDSP {
public:
  BreathDSP() 
    : _filtered(22.0f),
      _baseline(22.0f),
      _prevFiltered(22.0f),
      _lastUpdateMs(0),
      _exhaleStartMs(0),
      _inhaleStartMs(0),
      _lastCycleDurationMs(5000),
      _lastExhaleDurationMs(3000),
      _lastInhaleDurationMs(2000),
      _currentState(BREATH_IDLE),
      _smoothedRPM(12.0f),
      _coherence(75.0f),
      _initialized(false) {}

  void init(float initialTemp, float initialAmb) {
    _filtered = initialTemp;
    _baseline = initialTemp;
    _prevFiltered = initialTemp;
    _lastUpdateMs = millis();
    _initialized = true;
  }

  BreathMetrics process(float rawObj, float rawAmb, uint32_t nowMs) {
    if (!_initialized || isnan(rawObj)) {
      if (!isnan(rawObj)) init(rawObj, rawAmb);
      BreathMetrics m = {};
      m.stateStr = "CALIBRATING";
      return m;
    }

    float dt = (nowMs - _lastUpdateMs) / 1000.0f;
    if (dt <= 0.001f) dt = 0.04f;
    _lastUpdateMs = nowMs;

    // 1. Low-Pass Exponential Moving Average (EMA) to eliminate ADC thermal noise
    const float alpha = 0.20f;
    _filtered += alpha * (rawObj - _filtered);

    // 2. First Derivative (dT/dt) in °C/second
    float derivative = (_filtered - _prevFiltered) / dt;
    _prevFiltered = _filtered;

    // 3. Adaptive Baseline Tracking
    // Only adapt baseline when not in active exhalation, compensating for room drift
    float delta = _filtered - _baseline;
    const float breathThreshold = 0.15f; // +0.15°C rise indicates warm breath packet

    if (_currentState == BREATH_IDLE) {
      const float beta = 0.012f; // Slow baseline tracking
      _baseline += beta * (_filtered - _baseline);
    } else if (_currentState == BREATH_INHALE && delta < 0.08f) {
      const float beta = 0.03f; // Faster recovery towards ambient
      _baseline += beta * (_filtered - _baseline);
    }

    // 4. Finite State Machine (FSM) for Breath Cycle Classification
    if (_currentState == BREATH_IDLE) {
      if (delta >= breathThreshold && derivative > 0.05f) {
        _currentState = BREATH_EXHALE;
        _exhaleStartMs = nowMs;
        if (_inhaleStartMs > 0) {
          _lastInhaleDurationMs = nowMs - _inhaleStartMs;
        }
      }
    } 
    else if (_currentState == BREATH_EXHALE) {
      // Exhale transition: temperature peaks and begins descending
      if (derivative < -0.06f || delta < (breathThreshold * 0.8f)) {
        _currentState = BREATH_INHALE;
        _inhaleStartMs = nowMs;
        _lastExhaleDurationMs = nowMs - _exhaleStartMs;

        // Breath cycle completed (Exhale + Inhale)
        uint32_t cycleMs = _lastExhaleDurationMs + _lastInhaleDurationMs;
        if (cycleMs >= 1500 && cycleMs <= 18000) { // Valid human cycle: 3.3 to 40 RPM
          _lastCycleDurationMs = cycleMs;
          float instantRPM = 60000.0f / (float)cycleMs;
          // Smooth RPM with EMA
          _smoothedRPM = (_smoothedRPM * 0.65f) + (instantRPM * 0.35f);
          updateCoherence();
        }
      }
    } 
    else if (_currentState == BREATH_INHALE) {
      if (delta < 0.08f && abs(derivative) < 0.04f) {
        // Returned to baseline / rest pause
        if (nowMs - _inhaleStartMs > 500) {
          _currentState = BREATH_IDLE;
        }
      } else if (delta >= breathThreshold && derivative > 0.05f && (nowMs - _inhaleStartMs > 600)) {
        // Immediate next exhale without idle pause
        _currentState = BREATH_EXHALE;
        _lastInhaleDurationMs = nowMs - _inhaleStartMs;
        _exhaleStartMs = nowMs;
        
        uint32_t cycleMs = _lastExhaleDurationMs + _lastInhaleDurationMs;
        if (cycleMs >= 1500 && cycleMs <= 18000) {
          _lastCycleDurationMs = cycleMs;
          float instantRPM = 60000.0f / (float)cycleMs;
          _smoothedRPM = (_smoothedRPM * 0.65f) + (instantRPM * 0.35f);
          updateCoherence();
        }
      }
    }

    // Prepare Metrics Struct
    BreathMetrics m;
    m.rawTemp = rawObj;
    m.filteredTemp = _filtered;
    m.ambientTemp = rawAmb;
    m.baselineTemp = _baseline;
    m.deltaTemp = delta;
    m.derivative = derivative;
    m.state = _currentState;
    m.stateStr = (_currentState == BREATH_EXHALE) ? "EXHALE" : 
                 ((_currentState == BREATH_INHALE) ? "INHALE" : "IDLE");
    m.rpm = _smoothedRPM;
    m.exhaleSeconds = (float)_lastExhaleDurationMs / 1000.0f;
    m.inhaleSeconds = (float)_lastInhaleDurationMs / 1000.0f;
    m.ieRatio = (_lastInhaleDurationMs > 0) ? ((float)_lastExhaleDurationMs / (float)_lastInhaleDurationMs) : 1.0f;
    m.coherenceScore = _coherence;

    return m;
  }

private:
  float _filtered;
  float _baseline;
  float _prevFiltered;
  uint32_t _lastUpdateMs;

  uint32_t _exhaleStartMs;
  uint32_t _inhaleStartMs;
  uint32_t _lastCycleDurationMs;
  uint32_t _lastExhaleDurationMs;
  uint32_t _lastInhaleDurationMs;

  BreathState _currentState;
  float _smoothedRPM;
  float _coherence;
  bool _initialized;

  /**
   * @brief Evaluates vagus nerve activation and respiratory sinus arrhythmia coherence.
   * Rewards smooth rhythms, optimal cadence (6-12 RPM), and lengthened exhales (I:E ~ 1:1.5 - 1:2.5).
   */
  void updateCoherence() {
    float score = 50.0f;

    // 1. Cadence Score (Ideal relaxation rate is 5.5 to 10 RPM)
    if (_smoothedRPM >= 5.0f && _smoothedRPM <= 11.0f) {
      score += 25.0f;
    } else if (_smoothedRPM > 11.0f && _smoothedRPM <= 16.0f) {
      score += 15.0f;
    } else {
      score -= 10.0f;
    }

    // 2. Inhale to Exhale Ratio (Longer exhale stimulates parasympathetic vagal brake)
    float ratio = (float)_lastExhaleDurationMs / (float)max(1UL, _lastInhaleDurationMs);
    if (ratio >= 1.4f && ratio <= 2.6f) {
      score += 25.0f; // Target 1:2 ratio
    } else if (ratio >= 1.0f && ratio < 1.4f) {
      score += 10.0f;
    } else {
      score -= 5.0f;
    }

    score = constrain(score, 10.0f, 100.0f);
    _coherence = (_coherence * 0.7f) + (score * 0.3f);
  }
};
