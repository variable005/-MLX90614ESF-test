#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "HeatMapDSP.h"
#include "WebDashboard.h"

/**
 * Circuit Heat Mapper & PCB Hotspot Locator
 * Hardware: ESP8266 + MLX90614ESF
 * 
 * Network: Station Mode (Pico) + mDNS (http://heatmapper.local)
 */

// Wi-Fi Credentials
const char* STA_SSID = "Pico";
const char* STA_PASS = "12345678";

// Hardware & DSP Objects
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HeatMapDSP dsp;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const uint32_t SAMPLE_INTERVAL_MS = 30; // ~33 Hz sampling rate
uint32_t lastSampleTime = 0;
const int ONBOARD_LED = 2; // GPIO 2 (Active LOW)
bool sensorOnline = false;
bool wifiAnnounced = false;

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
  Serial.printf("[I2C] Probing %s (SDA=%d, SCL=%d)... ", name, sda, scl);
  Wire.begin(sda, scl);
  Wire.setClockStretchLimit(2000);
  Wire.setClock(50000);
  delay(80);

  if (mlx.begin()) {
    delay(40);
    double amb = mlx.readAmbientTempC();
    double obj = mlx.readObjectTempC();
    if (!isnan(amb) && !isnan(obj) && amb > -40.0 && amb < 125.0) {
      Serial.printf("SUCCESS!\n[SENSOR] Ambient: %.2f C | Object: %.2f C\n", (float)amb, (float)obj);
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
  digitalWrite(ONBOARD_LED, LOW); // LED ON while starting

  // 1. Initialize Sensor with Auto-Pin Detection
  sensorOnline = tryInitSensor(4, 5, "Config A (SDA=D2, SCL=D1)");
  if (!sensorOnline) {
    sensorOnline = tryInitSensor(5, 4, "Config B (SDA=D1, SCL=D2)");
  }

  // 2. Wi-Fi Configuration (Non-blocking background connection)
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.printf("[WIFI] Connecting to '%s' in background...\n", STA_SSID);
  WiFi.begin(STA_SSID, STA_PASS);

  // 3. Start Web and WebSocket Servers immediately
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[HTTP] Web server active on port 80");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WS] WebSocket active on port 81");

  // 4. mDNS responder: access via http://heatmapper.local
  if (MDNS.begin("heatmapper")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Responder active: http://heatmapper.local");
  }

  digitalWrite(ONBOARD_LED, HIGH); // LED OFF when servers ready
}

void loop() {
  server.handleClient();
  webSocket.loop();
  MDNS.update();

  // Announce Wi-Fi connection once established
  if (WiFi.status() == WL_CONNECTED && !wifiAnnounced) {
    wifiAnnounced = true;
    Serial.println("\n========================================");
    Serial.println("  [WIFI] CONNECTED TO PICO!");
    Serial.print("  [URL]  http://");
    Serial.println(WiFi.localIP());
    Serial.println("  [mDNS] http://heatmapper.local");
    Serial.println("========================================\n");
  }

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    float rawObj = 0.0f;
    float rawAmb = 0.0f;

    if (sensorOnline) {
      rawObj = (float)mlx.readObjectTempC();
      rawAmb = (float)mlx.readAmbientTempC();

      if (isnan(rawObj) || isnan(rawAmb) || rawAmb < -40.0f) {
        sensorOnline = false;
      }
    } else {
      // Background retry probe every 2 seconds
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
    if (now - lastSerial >= 400) {
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
