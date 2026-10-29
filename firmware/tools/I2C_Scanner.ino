#include <Wire.h>

/**
 * Diagnostic I2C Bus Scanner for ESP8266
 * Pins: D2 (SDA), D1 (SCL)
 */

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP8266 I2C Bus Scanner ---");
  Serial.println("SDA: D2 (GPIO 4), SCL: D1 (GPIO 5)");
  Wire.begin(4, 5);
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("\nScanning I2C bus...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  [✓] I2C device found at address 0x%02X", address);
      if (address == 0x5A) {
        Serial.print("  <-- MLX90614 Contactless IR Thermometer!");
      } else if (address == 0x3C || address == 0x3D) {
        Serial.print("  <-- OLED Display (SSD1306)");
      } else if (address == 0x57) {
        Serial.print("  <-- MAX30102 Pulse Oximeter");
      }
      Serial.println();
      nDevices++;
    } else if (error == 4) {
      Serial.printf("  [!] Unknown error at address 0x%02X\n", address);
    }
  }

  if (nDevices == 0) {
    Serial.println("  [X] No I2C devices found. Check 3.3V, GND, D1, D2 connections.");
  } else {
    Serial.printf("Scan complete. %d device(s) found.\n", nDevices);
  }

  delay(5000);
}
