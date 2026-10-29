#include <Wire.h>

/**
 * Hardware Bus Level & Electrical Diagnostic for ESP8266
 * Checks if SDA and SCL lines are physically pulled HIGH or stuck LOW.
 */

void checkBusLines(int sda, int scl, const char* label) {
  // 1. Measure raw pin electrical state with internal pull-up enabled
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, INPUT_PULLUP);
  delay(10);
  int sdaState = digitalRead(sda);
  int sclState = digitalRead(scl);

  Serial.printf("\n--- Electrical Check: %s ---\n", label);
  Serial.printf("  SDA (GPIO %d) idle state: %s\n", sda, (sdaState == HIGH) ? "HIGH (OK - 3.3V)" : "STUCK LOW (Short to GND or no power!)");
  Serial.printf("  SCL (GPIO %d) idle state: %s\n", scl, (sclState == HIGH) ? "HIGH (OK - 3.3V)" : "STUCK LOW (Short to GND or no power!)");

  if (sdaState == LOW || sclState == LOW) {
    Serial.println("  [DIAGNOSIS] I2C bus lines cannot communicate because one or both lines are grounded!");
    return;
  }

  // 2. Scan all I2C frequencies (10kHz, 50kHz, 100kHz)
  uint32_t clocks[] = {50000, 100000, 10000};
  for (int c = 0; c < 3; c++) {
    Wire.begin(sda, scl);
    Wire.setClockStretchLimit(2500);
    Wire.setClock(clocks[c]);
    delay(50);

    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      byte err = Wire.endTransmission();
      if (err == 0) {
        Serial.printf("  >>> SUCCESS! Found device at address 0x%02X (Clock: %u Hz)\n", addr, clocks[c]);
        found++;
      }
    }
    if (found > 0) return;
  }
  Serial.println("  [RESULT] Lines are electrically HIGH, but sensor did not send ACK byte.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("  ESP8266 I2C ELECTRICAL DIAGNOSTIC");
  Serial.println("========================================");
}

void loop() {
  // Test D2 (GPIO 4) as SDA, D1 (GPIO 5) as SCL
  checkBusLines(4, 5, "Config A: SDA=D2, SCL=D1");
  delay(1000);

  // Test D1 (GPIO 5) as SDA, D2 (GPIO 4) as SCL
  checkBusLines(5, 4, "Config B: SDA=D1, SCL=D2");
  delay(3000);
}
