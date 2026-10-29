#include <Arduino.h>
#include <Wire.h>

/**
 * Diagnostic I2C bus scanner for the ESP8266 heat mapper.
 *
 * Scans every pin pair the main firmware tries, at 100 kHz, 50 kHz, and
 * 10 kHz, then attempts a direct MLX90614 register read on 0x5A.
 *
 * Open this sketch in the Arduino IDE, select your ESP8266 board, and watch
 * the Serial Monitor at 115200 baud.
 */

struct BusConfig {
  int sda;
  int scl;
  const char* name;
};

static const BusConfig BUSES[] = {
  {4, 5, "D2(SDA)/D1(SCL)"},
  {5, 4, "D1(SDA)/D2(SCL)"},
  {12, 14, "D6(SDA)/D5(SCL)"},
  {14, 12, "D5(SDA)/D6(SCL)"},
};

static void scanBus(const BusConfig& bus, uint32_t clockHz) {
  Wire.begin(bus.sda, bus.scl);
  Wire.setClock(clockHz);
  Wire.setClockStretchLimit(25000);
  delay(40);

  pinMode(bus.sda, INPUT_PULLUP);
  pinMode(bus.scl, INPUT_PULLUP);
  delay(2);

  Serial.printf("\n[%s] %lu Hz  idle SDA=%d SCL=%d\n", bus.name,
                (unsigned long)clockHz, digitalRead(bus.sda),
                digitalRead(bus.scl));

  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  ACK 0x%02X", addr);
      if (addr == 0x3C || addr == 0x3D) Serial.print("  <-- SSD1306 OLED");
      if (addr == 0x5A) Serial.print("  <-- MLX90614");
      Serial.println();
      found++;
    } else if (err != 2) {
      Serial.printf("  error=%u at 0x%02X\n", err, addr);
    }
  }
  if (found == 0) Serial.println("  no devices");

  // Direct MLX90614 read: command byte, repeated start, 3 bytes (LSB, MSB, PEC)
  const uint8_t regs[] = {0x06, 0x07}; // ambient, object1
  for (uint8_t reg : regs) {
    Wire.beginTransmission(0x5A);
    Wire.write(reg);
    uint8_t err = Wire.endTransmission(false);
    uint8_t n = 0;
    uint8_t data[3] = {0, 0, 0};

    if (err == 0) {
      n = Wire.requestFrom((uint8_t)0x5A, (size_t)3, true);
      for (uint8_t i = 0; i < n && i < 3; i++) data[i] = Wire.read();
    }

    Serial.printf("  MLX reg 0x%02X: endErr=%u bytes=%u data=%02X %02X %02X\n",
                  reg, err, n, data[0], data[1], data[2]);
    delay(5);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  ESP8266 I2C / MLX90614 SCANNER");
  Serial.println("========================================");
}

void loop() {
  const uint32_t clocks[] = {100000, 50000, 10000};

  for (const BusConfig& bus : BUSES) {
    for (uint32_t hz : clocks) {
      scanBus(bus, hz);
    }
  }

  Serial.println("\n--- sweep complete, retrying in 5 s ---");
  delay(5000);
}
