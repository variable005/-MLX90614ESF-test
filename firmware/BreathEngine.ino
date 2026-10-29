#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "MLX90614_Driver.h"
#include "BreathDSP.h"
#include "WebDashboard.h"

/**
 * 🫁 Vagus Breath Engine & Contactless Spirometer
 * Hardware: ESP8266 + MLX90614ESF
 * 
 * Pinout:
 *   ESP8266 3.3V  -> MLX90614 VIN
 *   ESP8266 GND   -> MLX90614 GND
 *   ESP8266 D1(5) -> MLX90614 SCL
 *   ESP8266 D2(4) -> MLX90614 SDA
 */

// --- Wi-Fi Configuration ---
// The ESP8266 creates its own Access Point so you can connect from any phone or PC
const char* AP_SSID = "Breath-Engine-AP";
const char* AP_PASS = "breathewell"; // Set to "" for open network

// Optional: Connect to your home Wi-Fi (leave empty to use AP only)
const char* STA_SSID = "";
const char* STA_PASS = "";

// Hardware Objects
MLX90614_Sensor sensor;
BreathDSP dsp;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

// Sampling Timers
const uint32_t SAMPLE_INTERVAL_MS = 40; // 25 Hz sampling rate
uint32_t lastSampleTime = 0;
const int ONBOARD_LED = 2; // GPIO 2 (Active LOW on NodeMCU/D1 Mini)

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
  delay(200);
  Serial.println("\n\n========================================");
  Serial.println("  🫁 VAGUS BREATH ENGINE - ESP8266");
  Serial.println("========================================");

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH); // LED off

  // 1. Initialize I2C and MLX90614
  Serial.print("[I2C] Initializing MLX90614 (SDA=D2, SCL=D1)... ");
  if (!sensor.begin(4, 5)) {
    Serial.println("FAILED!");
    Serial.println("⚠️ Check wiring: 3.3V, GND, D1(SCL), D2(SDA)");
  } else {
    Serial.println("SUCCESS!");
    float amb = sensor.readAmbientTempC();
    float obj = sensor.readObjectTempC();
    Serial.printf("[SENSOR] Ambient: %.2f °C | Object: %.2f °C\n", amb, obj);
    dsp.init(obj, amb);
  }

  // 2. Configure Wi-Fi
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("[WIFI] Access Point started: ");
  Serial.println(AP_SSID);
  Serial.print("[WIFI] AP Web Dashboard URL: http://");
  Serial.println(WiFi.softAPIP());

  if (strlen(STA_SSID) > 0) {
    Serial.printf("[WIFI] Connecting to %s...", STA_SSID);
    WiFi.begin(STA_SSID, STA_PASS);
    uint32_t startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
      delay(300);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\n[WIFI] Connected! Station IP: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n[WIFI] Station connection timed out. Running in AP mode.");
    }
  }

  // 3. Web & WebSocket Servers
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[HTTP] Web server started on port 80");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSocket server started on port 81");

  Serial.println("\n🚀 System Ready! Breathe gently towards sensor (10-20 cm away).");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    // Read IR sensor
    float objTemp = sensor.readObjectTempC();
    float ambTemp = sensor.readAmbientTempC();

    // Process through real-time DSP
    BreathMetrics m = dsp.process(objTemp, ambTemp, now);

    // Visual LED feedback for exhale
    if (m.state == BREATH_EXHALE) {
      digitalWrite(ONBOARD_LED, LOW); // LED ON
    } else {
      digitalWrite(ONBOARD_LED, HIGH); // LED OFF
    }

    // Format compact JSON telemetry payload
    // { "delta": 0.65, "obj": 24.2, "amb": 23.5, "state": "EXHALE", "rpm": 12.1, "coherence": 85, "ie_ratio": 1.8, "exhale_s": 3.2 }
    char jsonBuffer[220];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
      "{\"delta\":%.2f,\"obj\":%.2f,\"amb\":%.2f,\"state\":\"%s\",\"rpm\":%.1f,\"coherence\":%.0f,\"ie_ratio\":%.1f,\"exhale_s\":%.1f}",
      m.deltaTemp, m.filteredTemp, m.ambientTemp, m.stateStr, m.rpm, m.coherenceScore, m.ieRatio, m.exhaleSeconds
    );

    // Broadcast to all connected phone/PC WebSocket clients
    webSocket.broadcastTXT(jsonBuffer);

    // Serial Plotter Output (open Tools -> Serial Plotter in Arduino IDE)
    // Format: Delta, Baseline, Filtered
    static uint32_t lastSerialPlot = 0;
    if (now - lastSerialPlot >= 100) {
      lastSerialPlot = now;
      Serial.printf("Delta:%.2f,Filtered:%.2f,Baseline:%.2f,State:%d,RPM:%.1f\n",
        m.deltaTemp, m.filteredTemp, m.baselineTemp, (int)m.state, m.rpm
      );
    }
  }
}
