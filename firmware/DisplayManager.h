#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HeatMapDSP.h"

/**
 * @file DisplayManager.h
 * @brief SSD1306 OLED UI for the Circuit Heat Mapper.
 *
 * Defaults to the common 0.96" 128x64 SSD1306 at 0x3C or 0x3D.
 * For a 128x32 panel, compile with -DOLED_HEIGHT=32 (or edit below).
 */

#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif

#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif

class HeatDisplay {
public:
  HeatDisplay()
    : _display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1),
      _ready(false),
      _addr(0) {}

  bool begin() {
    const uint8_t addresses[] = {0x3C, 0x3D};

    for (uint8_t i = 0; i < sizeof(addresses); i++) {
      if (_display.begin(SSD1306_SWITCHCAPVCC, addresses[i])) {
        _addr = addresses[i];
        _ready = true;
        _display.clearDisplay();
        _display.setTextColor(SSD1306_WHITE);
        _display.setTextWrap(false);
        _display.display();
        return true;
      }
    }
    return false;
  }

  bool ready() const { return _ready; }
  uint8_t address() const { return _addr; }

  void splash(const char* busName, bool mlxFound, uint8_t oledAddr) {
    if (!_ready) return;

    char line[28];
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.print(F("CIRCUIT HEAT MAPPER"));
    _display.drawLine(0, 9, OLED_WIDTH - 1, 9, SSD1306_WHITE);

    if (_display.height() >= 64) {
      _display.setCursor(0, 14);
      _display.print(F("ESP8266 + MLX90614"));
      _display.setCursor(0, 26);
      snprintf(line, sizeof(line), "OLED 0x%02X  OK", oledAddr);
      _display.print(line);
      _display.setCursor(0, 34);
      _display.print(mlxFound ? F("MLX 0x5A  OK") : F("MLX NOT FOUND"));
      _display.setCursor(0, 44);
      snprintf(line, sizeof(line), "Bus %s", busName);
      _display.print(line);
      _display.setCursor(0, 54);
      _display.print(F("Starting WiFi..."));
    } else {
      _display.setTextSize(2);
      _display.setCursor(0, 9);
      _display.print(F("HEAT MAP"));
      _display.setTextSize(1);
      _display.setCursor(0, 24);
      _display.print(mlxFound ? F("MLX OK") : F("MLX MISSING"));
    }
    _display.display();
  }

  void showLive(const HeatMetrics& m, const char* ipText) {
    if (!_ready) return;

    char line[28];
    bool compact = _display.height() < 64;
    _display.clearDisplay();
    drawHeader(ipText, compact && m.isHotspot);

    if (_display.height() >= 64) {
      _display.setTextSize(1);
      _display.setCursor(0, 12);
      _display.print(F("OBJECT"));

      char temp[8];
      snprintf(temp, sizeof(temp), "%.1f", m.filteredObj);
      _display.setTextSize(3);
      _display.setCursor(0, 21);
      _display.print(temp);

      int16_t cx = (int16_t)strlen(temp) * 18 + 4;
      if (cx > OLED_WIDTH - 12) cx = OLED_WIDTH - 12;
      _display.setTextSize(2);
      _display.setCursor(cx, 27);
      _display.print(F("C"));

      _display.setTextSize(1);
      _display.setCursor(0, 46);
      snprintf(line, sizeof(line), "AMB %.1f  dT %+.1f", m.rawAmb, m.delta);
      _display.print(line);

      if (m.isHotspot) {
        _display.fillRect(0, 54, OLED_WIDTH, 10, SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK);
      }
      _display.setCursor(0, 55);
      snprintf(line, sizeof(line), "PEAK %.1f  %s", m.peakTemp,
               m.isHotspot ? "HOT!" : "OK");
      _display.print(line);
      _display.setTextColor(SSD1306_WHITE);
    } else {
      _display.setTextSize(2);
      _display.setCursor(0, 9);
      snprintf(line, sizeof(line), "%.1fC", m.filteredObj);
      _display.print(line);

      int16_t x = (int16_t)strlen(line) * 12 + 4;
      if (x > 90) x = 90;
      _display.setTextSize(1);
      _display.setCursor(x, 9);
      _display.print(F("OBJ"));

      _display.setCursor(0, 24);
      snprintf(line, sizeof(line), "A%.1f dT%+.1f P%.1f", m.rawAmb, m.delta,
               m.peakTemp);
      _display.print(line);
    }

    _display.display();
  }

  void showSensorMissing(const char* sensorBusName, const char* displayBusName,
                         uint8_t oledAddr, bool mlxSeen, const char* ipText) {
    if (!_ready) return;

    char line[28];
    _display.clearDisplay();
    drawHeader(ipText);

    if (_display.height() >= 64) {
      _display.setTextSize(2);
      _display.setCursor(0, 14);
      _display.print(F("MLX90614"));

      _display.setTextSize(1);
      _display.setCursor(0, 32);
      _display.print(mlxSeen ? F("FOUND / READ FAIL") : F("NOT DETECTED"));

      _display.setCursor(0, 40);
      snprintf(line, sizeof(line), "MLX %s", sensorBusName);
      _display.print(line);

      _display.setCursor(0, 48);
      snprintf(line, sizeof(line), "OLED %s 0x%02X", displayBusName, oledAddr);
      _display.print(line);

      _display.setCursor(0, 56);
      _display.print(F("Check MLX wiring"));
    } else {
      _display.setTextSize(2);
      _display.setCursor(0, 9);
      _display.print(F("NO MLX"));
      _display.setTextSize(1);
      _display.setCursor(0, 24);
      snprintf(line, sizeof(line), "MLX %s", sensorBusName);
      _display.print(line);
    }

    _display.display();
  }

private:
  Adafruit_SSD1306 _display;
  bool _ready;
  uint8_t _addr;

  void drawHeader(const char* ipText, bool alert = false) {
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.print(F("HEAT"));

    if (alert) {
      _display.setCursor(OLED_WIDTH - 24, 0);
      _display.print(F("HOT!"));
    } else if (ipText && ipText[0]) {
      int16_t x = OLED_WIDTH - (int16_t)strlen(ipText) * 6;
      if (x < 34) x = 34;
      _display.setCursor(x, 0);
      _display.print(ipText);
    } else {
      _display.setCursor(OLED_WIDTH - 42, 0);
      _display.print(F("no WiFi"));
    }

    _display.drawLine(0, 9, OLED_WIDTH - 1, 9, SSD1306_WHITE);
  }
};
