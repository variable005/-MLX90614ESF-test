#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "MLX90614_Driver.h"
#include "HeatMapDSP.h"
#include "WebDashboard.h"

/**
 * Circuit Heat Mapper & PCB Hotspot Locator
 * Hardware: ESP8266 + MLX90614ESF
 * 
 * Pinout:
 *   ESP8266 3.3V  -> MLX90614 VIN
 *   ESP8266 GND   -> MLX90614 GND
 *   ESP8266 D1(5) -> MLX90614 SCL
 *   ESP8266 D2(4) -> MLX90614 SDA
 */

// Wi-Fi Credentials
const char* STA_SSID = "Pico";
const char* STA_PASS = "12345678";

// Standalone Fallback Access Point
const char* AP_SSID  = "HeatMapper-AP";
const char* AP_PASS  = "12345678";

// Hardware & DSP Objects
MLX90614_Sensor sensor;
HeatMapDSP dsp;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const uint32_t SAMPLE_INTERVAL_MS = 25; // 40 Hz sampling rate
uint32_t lastSampleTime = 0;
const int ONBOARD_LED = 2; // GPIO 2 (Active LOW)

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

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n========================================");
  Serial.println("  CIRCUIT HEAT MAPPER - ESP8266");
  Serial.println("========================================");

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW); // LED ON while booting

  // 1. Initialize I2C and MLX90614
  Serial.print("[I2C] Initializing MLX90614 (SDA=D2, SCL=D1)... ");
  if (!sensor.begin(4, 5)) {
    Serial.println("FAILED!");
    Serial.println("[ERROR] Check 3.3V, GND, D1(SCL), D2(SDA) wiring.");
  } else {
    Serial.println("SUCCESS!");
    float amb = sensor.readAmbientTempC();
    float obj = sensor.readObjectTempC();
    Serial.printf("[SENSOR] Baseline Ambient: %.2f C | Initial Object: %.2f C\n", amb, obj);
    dsp.init(obj);
  }

  // 2. Connect to Wi-Fi Network
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.printf("[WIFI] Connecting to '%s'...", STA_SSID);
  WiFi.begin(STA_SSID, STA_PASS);

  uint32_t startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 10000)) {
    delay(400);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected successfully!");
    Serial.print("[HTTP] Circuit Heat Mapper Web URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Station connection timed out. Running in standalone AP mode.");
    Serial.print("[HTTP] Fallback Web URL: http://");
    Serial.println(WiFi.softAPIP());
  }

  // 3. Start Web and WebSocket Servers
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[HTTP] Web server started on port 80");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSocket server started on port 81");

  digitalWrite(ONBOARD_LED, HIGH); // LED OFF when ready
  Serial.println("\n[READY] Hold sensor 5mm - 10mm above PCB components to map heat.");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    // Read sensor
    float rawObj = sensor.readObjectTempC();
    float rawAmb = sensor.readAmbientTempC();

    // Process DSP metrics
    HeatMetrics m = dsp.process(rawObj, rawAmb, now);

    // LED alert on hotspot
    if (m.isHotspot) {
      digitalWrite(ONBOARD_LED, LOW); // LED ON
    } else {
      digitalWrite(ONBOARD_LED, HIGH); // LED OFF
    }

    // Broadcast compact JSON telemetry packet to web UI
    char jsonBuffer[140];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
      "{\"t_obj\":%.2f,\"t_amb\":%.2f,\"delta\":%.2f,\"rate\":%.2f,\"peak\":%.2f}",
      m.filteredObj, m.rawAmb, m.delta, m.rateOfChange, m.peakTemp
    );
    webSocket.broadcastTXT(jsonBuffer);

    // Periodic Serial output for debugging
    static uint32_t lastSerial = 0;
    if (now - lastSerial >= 250) {
      lastSerial = now;
      Serial.printf("Obj: %.2f C | Amb: %.2f C | Delta: +%.2f C | Peak: %.2f C\n",
        m.filteredObj, m.rawAmb, m.delta, m.peakTemp
      );
    }
  }
}
