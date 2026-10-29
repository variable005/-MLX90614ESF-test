#include <Wire.h>

/**
 * Diagnostic I2C Bus Scanner for ESP8266
 * Tests both pin assignments:
 * Config A: SDA=D2 (GPIO 4), SCL=D1 (GPIO 5)
 * Config B: SDA=D1 (GPIO 5), SCL=D2 (GPIO 4)
 */

void scan(int sda, int scl, const char* label) {
  Serial.printf("\n--- Scanning %s (SDA=%d, SCL=%d) ---\n", label, sda, scl);
  Wire.begin(sda, scl);
  delay(100);

  int nDevices = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  [FOUND] Device at 0x%02X", address);
      if (address == 0x5A) Serial.print(" <-- MLX90614 Infrared Thermometer!");
      if (address == 0x3C || address == 0x3D) Serial.print(" <-- OLED Display");
      if (address == 0x57) Serial.print(" <-- MAX30102 Pulse Oximeter");
      Serial.println();
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("  [NONE] No devices detected.");
  } else {
    Serial.printf("  Total %d device(s) found.\n", nDevices);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== DUAL PIN I2C BUS SCANNER ===");
}

void loop() {
  // Test Config A: SDA=D2 (GPIO 4), SCL=D1 (GPIO 5)
  scan(4, 5, "Config A: SDA=D2, SCL=D1");
  delay(1000);

  // Test Config B: SDA=D1 (GPIO 5), SCL=D2 (GPIO 4)
  scan(5, 4, "Config B: SDA=D1, SCL=D2");
  delay(3000);
}
