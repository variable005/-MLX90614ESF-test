#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "HeatMapDSP.h"
#include "WebDashboard.h"

/**
 * Circuit Heat Mapper & PCB Hotspot Locator
 * Hardware: ESP8266 + MLX90614ESF
 * 
 * Auto-detects pinout on D1 & D2:
 * Config A: SDA=D2 (GPIO 4), SCL=D1 (GPIO 5)
 * Config B: SDA=D1 (GPIO 5), SCL=D2 (GPIO 4)
 */

// Wi-Fi Credentials
const char* STA_SSID = "Pico";
const char* STA_PASS = "12345678";

// Standalone Fallback Access Point
const char* AP_SSID  = "HeatMapper-AP";
const char* AP_PASS  = "12345678";

// Hardware & DSP Objects
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HeatMapDSP dsp;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const uint32_t SAMPLE_INTERVAL_MS = 30; // ~33 Hz sampling rate
uint32_t lastSampleTime = 0;
const int ONBOARD_LED = 2; // GPIO 2 (Active LOW)
bool sensorOnline = false;

void handleRoot() {
  server.send_P(200, "text/html", PAGE_INDEX);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[WS] Client #%u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("[WS] Client #%u disconnected\n", num);
  }
}

bool tryInitSensor(int sda, int scl, const char* name) {
  Serial.printf("[I2C] Trying %s (SDA=%d, SCL=%d)... ", name, sda, scl);
  Wire.begin(sda, scl);
  Wire.setClockStretchLimit(2000); // Crucial for MLX90614 SMBus clock stretching
  Wire.setClock(50000); // 50kHz SMBus friendly clock
  delay(100);

  if (mlx.begin()) {
    delay(50);
    double amb = mlx.readAmbientTempC();
    double obj = mlx.readObjectTempC();
    if (!isnan(amb) && !isnan(obj) && amb > -40.0 && amb < 125.0) {
      Serial.printf("SUCCESS!\n[SENSOR] Baseline Ambient: %.2f C | Object: %.2f C\n", (float)amb, (float)obj);
      dsp.init((float)obj);
      return true;
    }
  }
  Serial.println("FAILED.");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n========================================");
  Serial.println("  CIRCUIT HEAT MAPPER - ESP8266");
  Serial.println("========================================");

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW); // LED ON while booting

  // 1. Initialize Sensor with Auto-Pin Detection
  // Try Config A: SDA=D2 (GPIO 4), SCL=D1 (GPIO 5)
  sensorOnline = tryInitSensor(4, 5, "Config A: SDA=D2, SCL=D1");
  if (!sensorOnline) {
    // Try Config B: SDA=D1 (GPIO 5), SCL=D2 (GPIO 4)
    sensorOnline = tryInitSensor(5, 4, "Config B: SDA=D1, SCL=D2");
  }

  if (!sensorOnline) {
    Serial.println("[WARNING] Sensor not responding on D1/D2 yet. Loop will continuously attempt reconnect.");
  }

  // 2. Wi-Fi Configuration
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.printf("[WIFI] Connecting to '%s'...", STA_SSID);
  WiFi.begin(STA_SSID, STA_PASS);

  uint32_t startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 8000)) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected to Pico!");
    Serial.print("[HTTP] Circuit Heat Mapper Web URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Running in standalone AP mode.");
    Serial.print("[HTTP] Connect to Wi-Fi 'HeatMapper-AP' (pass: 12345678) -> URL: http://");
    Serial.println(WiFi.softAPIP());
  }

  // 3. Start Web and WebSocket Servers
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[HTTP] Web server active on port 80");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSocket active on port 81");

  digitalWrite(ONBOARD_LED, HIGH); // LED OFF when ready
  Serial.println("\n[READY] System operational. Web dashboard ready to map heat.");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    float rawObj = 0.0f;
    float rawAmb = 0.0f;

    if (sensorOnline) {
      rawObj = (float)mlx.readObjectTempC();
      rawAmb = (float)mlx.readAmbientTempC();

      // Check for bus drop
      if (isnan(rawObj) || isnan(rawAmb) || rawAmb < -40.0f) {
        sensorOnline = false;
      }
    } else {
      // Background retry probe
      static uint32_t lastRetry = 0;
      if (now - lastRetry > 2000) {
        lastRetry = now;
        sensorOnline = tryInitSensor(4, 5, "Config A");
        if (!sensorOnline) sensorOnline = tryInitSensor(5, 4, "Config B");
      }
    }

    // Process DSP metrics
    HeatMetrics m = dsp.process(rawObj, rawAmb, now);

    // Visual LED feedback for hotspots
    if (m.isHotspot) {
      digitalWrite(ONBOARD_LED, LOW); // LED ON
    } else {
      digitalWrite(ONBOARD_LED, HIGH); // LED OFF
    }

    // Broadcast JSON telemetry to browser
    char jsonBuffer[140];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
      "{\"t_obj\":%.2f,\"t_amb\":%.2f,\"delta\":%.2f,\"rate\":%.2f,\"peak\":%.2f}",
      m.filteredObj, m.rawAmb, m.delta, m.rateOfChange, m.peakTemp
    );
    webSocket.broadcastTXT(jsonBuffer);

    // Periodic Serial Plotter trace
    static uint32_t lastSerial = 0;
    if (now - lastSerial >= 300) {
      lastSerial = now;
      if (sensorOnline) {
        Serial.printf("Obj: %.2f C | Amb: %.2f C | Delta: +%.2f C | Peak: %.2f C\n",
          m.filteredObj, m.rawAmb, m.delta, m.peakTemp
        );
      } else {
        Serial.println("[STATUS] Waiting for MLX90614 sensor on D1/D2...");
      }
    }
  }
}
