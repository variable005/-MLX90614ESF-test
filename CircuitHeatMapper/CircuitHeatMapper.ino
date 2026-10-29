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
bool mdnsStarted = false;

bool isValidReading(float obj, float amb) {
  // MLX90614 datasheet range: object -70..382 C, ambient -40..125 C
  return !isnan(obj) && !isnan(amb) &&
         obj >= -70.0f && obj <= 382.0f &&
         amb >= -40.0f && amb <= 125.0f;
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE_INDEX);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  (void)payload;
  (void)length;

  if (type == WStype_CONNECTED) {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[WS] Client #%u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);

    // Tell a newly connected dashboard immediately whether the sensor is failing.
    if (!sensorOnline) {
      webSocket.sendTXT(num, "{\"online\":false}");
    }
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("[WS] Client #%u disconnected\n", num);
  }
}

bool tryInitSensor(int sda, int scl, const char* name) {
  Serial.printf("[I2C] Probing %s (SDA=%d, SCL=%d)... ", name, sda, scl);
  Wire.begin(sda, scl);
  Wire.setClockStretchLimit(10000);
  Wire.setClock(50000);
  delay(60);

  // Initialize library instance
  mlx.begin();
  delay(40);

  // Directly attempt reading temperature registers (handles SMBus sensors that reject 0-byte probe scans)
  float amb = (float)mlx.readAmbientTempC();
  float obj = (float)mlx.readObjectTempC();
  if (isValidReading(obj, amb)) {
    Serial.printf("SUCCESS!\n[SENSOR] Ambient: %.2f C | Object: %.2f C\n", amb, obj);
    dsp.init(obj);
    return true;
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
  WiFi.persistent(false); // Do not wear the flash with repeated credential saves
  WiFi.mode(WIFI_STA);
  WiFi.hostname("heatmapper");
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

  digitalWrite(ONBOARD_LED, HIGH); // LED OFF when servers ready
}

void loop() {
  server.handleClient();
  webSocket.loop();
  if (mdnsStarted) MDNS.update();

  // mDNS must be started only after the station has an IP address.
  if (WiFi.status() == WL_CONNECTED && !wifiAnnounced) {
    wifiAnnounced = true;
    Serial.println("\n========================================");
    Serial.println("  [WIFI] CONNECTED TO PICO!");
    Serial.print("  [URL]  http://");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("heatmapper")) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      Serial.println("  [mDNS] http://heatmapper.local");
    } else {
      Serial.println("  [mDNS] Responder failed to start");
    }
    Serial.println("========================================\n");
  }

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    if (sensorOnline) {
      float rawObj = (float)mlx.readObjectTempC();
      float rawAmb = (float)mlx.readAmbientTempC();

      if (!isValidReading(rawObj, rawAmb)) {
        // A failed read must not be fed into the DSP as 0 C.
        sensorOnline = false;
        digitalWrite(ONBOARD_LED, HIGH); // LED OFF
        webSocket.broadcastTXT("{\"online\":false}");
        Serial.println("[SENSOR] Read failed - entering retry mode.");
      } else {
        // Process DSP metrics
        HeatMetrics m = dsp.process(rawObj, rawAmb, now);

        // Visual LED feedback for hotspots
        if (m.isHotspot) {
          digitalWrite(ONBOARD_LED, LOW); // LED ON
        } else {
          digitalWrite(ONBOARD_LED, HIGH); // LED OFF
        }

        // Broadcast JSON telemetry to browser
        char jsonBuffer[160];
        snprintf(jsonBuffer, sizeof(jsonBuffer),
          "{\"online\":true,\"t_obj\":%.2f,\"t_amb\":%.2f,\"delta\":%.2f,\"rate\":%.2f,\"peak\":%.2f}",
          m.filteredObj, m.rawAmb, m.delta, m.rateOfChange, m.peakTemp
        );
        webSocket.broadcastTXT(jsonBuffer);

        // Periodic Serial Plotter trace
        static uint32_t lastSerial = 0;
        if (now - lastSerial >= 400) {
          lastSerial = now;
          Serial.printf("Obj: %.2f C | Amb: %.2f C | Delta: +%.2f C | Peak: %.2f C\n",
            m.filteredObj, m.rawAmb, m.delta, m.peakTemp
          );
        }
      }
    } else {
      // Background retry probe every 2 seconds
      static uint32_t lastRetry = 0;
      if (now - lastRetry > 2000) {
        lastRetry = now;
        sensorOnline = tryInitSensor(4, 5, "Config A");
        if (!sensorOnline) sensorOnline = tryInitSensor(5, 4, "Config B");
      }

      // Keep dashboards informed without spamming the socket at 33 Hz.
      static uint32_t lastOfflineNotice = 0;
      if (!sensorOnline && now - lastOfflineNotice >= 1000) {
        lastOfflineNotice = now;
        webSocket.broadcastTXT("{\"online\":false}");
        Serial.println("[STATUS] Waiting for MLX90614 sensor on D1/D2...");
      }
    }
  }
}
