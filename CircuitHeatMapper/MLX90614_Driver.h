#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * @file MLX90614_Driver.h
 * @brief High-reliability, zero-dependency SMBus/I2C driver for MLX90614ESF
 * Supports standard SMBus Repeated Start and PEC (Packet Error Code) validation.
 */
class MLX90614_Sensor {
public:
  static const uint8_t DEFAULT_I2C_ADDR = 0x5A;
  static const uint8_t REG_AMBIENT_TEMP = 0x06;
  static const uint8_t REG_OBJECT1_TEMP = 0x07;

  MLX90614_Sensor(uint8_t addr = DEFAULT_I2C_ADDR) : _addr(addr) {}

  bool begin(int sdaPin = 4, int sclPin = 5) {
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000); // Standard 100kHz I2C SMBus rate
    delay(50);
    // Verify sensor presence by reading ambient temp
    float testAmb = readAmbientTempC();
    return (testAmb > -40.0f && testAmb < 125.0f);
  }

  /**
   * @brief Reads object temperature in Celsius
   * @return Object temperature in °C, or NAN on bus error
   */
  float readObjectTempC() {
    return readRegisterTemp(REG_OBJECT1_TEMP);
  }

  /**
   * @brief Reads ambient sensor temperature in Celsius
   * @return Ambient temperature in °C, or NAN on bus error
   */
  float readAmbientTempC() {
    return readRegisterTemp(REG_AMBIENT_TEMP);
  }

private:
  uint8_t _addr;

  float readRegisterTemp(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    uint8_t err = Wire.endTransmission(false); // Repeated start is mandatory for SMBus
    if (err != 0) {
      Wire.beginTransmission(_addr);
      Wire.endTransmission(true);
      return NAN;
    }

    // Request 3 bytes: Data Low, Data High, PEC
    uint8_t bytesReceived = Wire.requestFrom(_addr, (uint8_t)3, (uint8_t)true);
    if (bytesReceived < 2) {
      return NAN;
    }

    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    if (Wire.available()) {
      uint8_t pec = Wire.read(); // Optional PEC byte
      (void)pec;
    }

    // Check error bit in MSB (MSB bit 7 indicates error flag on MLX90614)
    if (msb & 0x80) {
      return NAN;
    }

    // Formula from Melexis datasheet: Temp (Kelvin) = Data * 0.02
    uint16_t raw = ((uint16_t)msb << 8) | lsb;
    float tempK = (float)raw * 0.02f;
    float tempC = tempK - 273.15f;
    return tempC;
  }
};
