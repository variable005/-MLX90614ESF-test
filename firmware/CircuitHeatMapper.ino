#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "HeatMapDSP.h"
#include "DisplayManager.h"
#include "WebDashboard.h"

/**
 * Circuit Heat Mapper & PCB Hotspot Locator
 * Hardware: ESP8266 + MLX90614ESF + SSD1306 OLED
 *
 * Network: Station Mode (Pico) + mDNS (http://heatmapper.local)
 *
 * The firmware scans four common I2C pin pairs. The OLED and MLX90614 may be
 * on the same pair or on two different pairs: the display bus and the sensor
 * bus are tracked independently and the I2C peripheral is switched as needed.
 */

// Wi-Fi Credentials
const char* STA_SSID = "Pico";
const char* STA_PASS = "12345678";

// Hardware & DSP Objects
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HeatMapDSP dsp;
HeatDisplay oled;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const uint32_t SAMPLE_INTERVAL_MS = 30;    // ~33 Hz sampling rate
const uint32_t DISPLAY_INTERVAL_MS = 250;  // OLED refresh on a shared bus
const uint32_t DISPLAY_SEPARATE_MS = 500;  // OLED refresh when bus switching
const uint32_t SENSOR_RETRY_MS = 2500;
const int ONBOARD_LED = 2; // GPIO 2 (Active LOW)

bool sensorOnline = false;
bool wifiAnnounced = false;
bool mdnsStarted = false;
bool mlxHintPrinted = false;
bool mlxEverSeen = false;
uint32_t lastSampleTime = 0;
uint32_t lastDisplayTime = 0;

// ---------------------------------------------------------------------------
// I2C bus discovery
// ---------------------------------------------------------------------------

struct I2cBusConfig {
  int sda;
  int scl;
  const char* name;
};

static const I2cBusConfig BUS_CONFIGS[] = {
  {4, 5, "D2/D1"},
  {5, 4, "D1/D2"},
  {12, 14, "D6/D5"},
  {14, 12, "D5/D6"},
};
static const uint8_t BUS_COUNT = sizeof(BUS_CONFIGS) / sizeof(BUS_CONFIGS[0]);

struct BusInfo {
  bool hasMlx;
  bool hasOled;
  uint8_t oledAddr;
};

BusInfo busInfo[BUS_COUNT];

int sensorSda = 4;
int sensorScl = 5;
const char* sensorBusName = "D2/D1";

int displaySda = 4;
int displayScl = 5;
const char* displayBusName = "D2/D1";
bool displayOnSeparateBus = false;

uint8_t activeMlxAddr = 0x5A;
uint8_t activeOledAddr = 0;

// Tracks which pin pair the ESP8266 I2C peripheral is currently on.
int currentSda = -1;
int currentScl = -1;

bool isValidReading(float obj, float amb) {
  // MLX90614 datasheet range: object -70..382 C, ambient -40..125 C
  return !isnan(obj) && !isnan(amb) &&
         obj >= -70.0f && obj <= 382.0f &&
         amb >= -40.0f && amb <= 125.0f;
}

bool addressInList(const uint8_t* list, uint8_t count, uint8_t addr) {
  for (uint8_t i = 0; i < count; i++) {
    if (list[i] == addr) return true;
  }
  return false;
}

void ensureBus(int sda, int scl) {
  if (currentSda == sda && currentScl == scl) return;

  Wire.begin(sda, scl);
  Wire.setClock(100000);
  Wire.setClockStretchLimit(10000);
  delay(30);

  currentSda = sda;
  currentScl = scl;
}

// MLX90614 wake-up request: SCL high, SDA low for >33 ms. This recovers a
// sensor that was previously put into sleep mode. Do not call this on the
// OLED bus before the display has been initialized.
void wakeBus(int sda, int scl) {
  pinMode(sda, OUTPUT);
  pinMode(scl, OUTPUT);
  digitalWrite(scl, HIGH);
  digitalWrite(sda, LOW);
  delay(50);
  digitalWrite(sda, HIGH);
  delay(10);
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, INPUT_PULLUP);
  delay(10);
}

int scanBus(int sda, int scl, uint8_t* addrs, uint8_t maxAddrs) {
  ensureBus(sda, scl);

  uint8_t n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      if (n < maxAddrs) addrs[n] = a;
      n++;
    }
  }
  return n;
}

void discoverBuses() {
  int mlxIndex = -1;
  int oledIndex = -1;

  Serial.println("[I2C] Scanning candidate buses...");

  for (uint8_t i = 0; i < BUS_COUNT; i++) {
    uint8_t addrs[8];
    uint8_t n = scanBus(BUS_CONFIGS[i].sda, BUS_CONFIGS[i].scl, addrs, sizeof(addrs));

    busInfo[i].hasMlx = addressInList(addrs, n, activeMlxAddr);
    busInfo[i].hasOled = addressInList(addrs, n, 0x3C) ||
                         addressInList(addrs, n, 0x3D);
    busInfo[i].oledAddr = addressInList(addrs, n, 0x3C)
                              ? 0x3C
                              : (addressInList(addrs, n, 0x3D) ? 0x3D : 0);

    Serial.printf("[I2C] %s (SDA=%d, SCL=%d): %u device(s)", BUS_CONFIGS[i].name,
                  BUS_CONFIGS[i].sda, BUS_CONFIGS[i].scl, n);
    for (uint8_t j = 0; j < n && j < sizeof(addrs); j++) {
      Serial.printf(" 0x%02X", addrs[j]);
    }
    Serial.printf("%s%s\n", busInfo[i].hasMlx ? " [MLX]" : "",
                  busInfo[i].hasOled ? " [OLED]" : "");

    if (busInfo[i].hasMlx && mlxIndex < 0) mlxIndex = i;
    if (busInfo[i].hasOled && oledIndex < 0) oledIndex = i;
  }

  // Sensor bus: prefer a bus where the MLX actually answered.
  if (mlxIndex >= 0) {
    sensorSda = BUS_CONFIGS[mlxIndex].sda;
    sensorScl = BUS_CONFIGS[mlxIndex].scl;
    sensorBusName = BUS_CONFIGS[mlxIndex].name;
  } else {
    // Nothing found yet: default to the standard NodeMCU I2C pair. The retry
    // loop scans every pair, so it will recover if the sensor is elsewhere.
    sensorSda = 4;
    sensorScl = 5;
    sensorBusName = "D2/D1";
  }

  // Display bus: independent of the sensor bus.
  if (oledIndex >= 0) {
    displaySda = BUS_CONFIGS[oledIndex].sda;
    displayScl = BUS_CONFIGS[oledIndex].scl;
    displayBusName = BUS_CONFIGS[oledIndex].name;
    activeOledAddr = busInfo[oledIndex].oledAddr;
  } else {
    displaySda = sensorSda;
    displayScl = sensorScl;
    displayBusName = sensorBusName;
    activeOledAddr = 0;
  }

  displayOnSeparateBus = (sensorSda != displaySda || sensorScl != displayScl);
  mlxEverSeen = (mlxIndex >= 0);

  Serial.printf("[I2C] Sensor bus %s, display bus %s%s\n", sensorBusName,
                displayBusName, displayOnSeparateBus ? " (separate)" : " (shared)");

  if (mlxIndex < 0 && oledIndex >= 0) {
    Serial.println("[I2C] MLX90614 not found yet; scanning all buses while offline.");
  }
}

bool scanForMlx() {
  for (uint8_t i = 0; i < BUS_COUNT; i++) {
    uint8_t addrs[8];
    uint8_t n = scanBus(BUS_CONFIGS[i].sda, BUS_CONFIGS[i].scl, addrs, sizeof(addrs));

    if (addressInList(addrs, n, activeMlxAddr)) {
      if (sensorSda != BUS_CONFIGS[i].sda || sensorScl != BUS_CONFIGS[i].scl) {
        Serial.printf("[I2C] MLX90614 found on %s; switching sensor bus.\n",
                      BUS_CONFIGS[i].name);
      }
      sensorSda = BUS_CONFIGS[i].sda;
      sensorScl = BUS_CONFIGS[i].scl;
      sensorBusName = BUS_CONFIGS[i].name;
      displayOnSeparateBus = (sensorSda != displaySda || sensorScl != displayScl);
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Device initialization
// ---------------------------------------------------------------------------

bool initMlx() {
  Serial.printf("[MLX] Probing 0x%02X on %s... ", activeMlxAddr, sensorBusName);

  ensureBus(sensorSda, sensorScl);
  bool acked = mlx.begin();
  Wire.setClock(100000);
  Wire.setClockStretchLimit(10000);
  delay(30);

  float amb = (float)mlx.readAmbientTempC();
  float obj = (float)mlx.readObjectTempC();

  if (isValidReading(obj, amb)) {
    Serial.printf("OK (Amb %.2f C, Obj %.2f C)\n", amb, obj);
    mlxEverSeen = true;
    dsp.init(obj);
    return true;
  }

  if (acked) {
    Serial.println("FAILED (device ACKs, but no valid temperature).");
  } else {
    Serial.println("FAILED (no ACK at 0x5A).");
  }

  if (!mlxHintPrinted) {
    mlxHintPrinted = true;
    Serial.println("  MLX90614 is not answering at 0x5A. Check:");
    Serial.println("   - VDD: 3.3V for -Bxx parts, 5V for -Axx parts (check suffix)");
    Serial.println("   - HW-691/GY-906 breakout: power VCC from 5V (VIN); the");
    Serial.println("     onboard 3.3V regulator can brown out at 3.3V input");
    Serial.println("   - GND connected to ESP8266 GND");
    Serial.println("   - SDA/SCL not swapped. Bare TO-39: 1=SCL, 2=SDA, 3=VDD, 4=GND");
    Serial.println("   - 4.7k pull-ups from SDA and SCL to 3.3V");
    Serial.println("   - HW-691/GY-906 modules: VCC, GND, SCL, SDA (check labels)");
  }
  return false;
}

bool initOled() {
  if (activeOledAddr == 0) {
    Serial.println("[OLED] No SSD1306 found on any candidate bus.");
    return false;
  }

  ensureBus(displaySda, displayScl);
  if (!oled.begin()) {
    Serial.println("[OLED] SSD1306 begin() failed.");
    if (displayOnSeparateBus) ensureBus(sensorSda, sensorScl);
    return false;
  }

  // The display library changes the bus speed during init; restore MLX-safe
  // settings afterwards.
  Wire.setClock(100000);
  Wire.setClockStretchLimit(10000);
  delay(20);

  Serial.printf("[OLED] SSD1306 ready at 0x%02X on %s\n", oled.address(),
                displayBusName);
  oled.splash(displayBusName, sensorOnline, oled.address());
  delay(1200);

  // Get back to the sensor bus so the next MLX read is immediate.
  if (displayOnSeparateBus) ensureBus(sensorSda, sensorScl);
  return true;
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

void getIpText(char* buf, size_t len) {
  buf[0] = '\0';
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    snprintf(buf, len, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  }
}

void showLiveOnDisplay(const HeatMetrics& m, const char* ipText) {
  if (!oled.ready()) return;

  ensureBus(displaySda, displayScl);
  oled.showLive(m, ipText);
  if (displayOnSeparateBus) ensureBus(sensorSda, sensorScl);
}

void showMissingOnDisplay(const char* ipText) {
  if (!oled.ready()) return;

  ensureBus(displaySda, displayScl);
  oled.showSensorMissing(sensorBusName, displayBusName, oled.address(),
                         mlxEverSeen, ipText);
  if (displayOnSeparateBus) ensureBus(sensorSda, sensorScl);
}

// ---------------------------------------------------------------------------
// Web server / WebSocket
// ---------------------------------------------------------------------------

void handleRoot() {
  server.send_P(200, "text/html", PAGE_INDEX);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)payload;
  (void)length;

  if (type == WStype_CONNECTED) {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[WS] Client #%u connected from %d.%d.%d.%d\n", num, ip[0],
                  ip[1], ip[2], ip[3]);

    if (!sensorOnline) {
      webSocket.sendTXT(num, "{\"online\":false}");
    }
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("[WS] Client #%u disconnected\n", num);
  }
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n========================================");
  Serial.println("  CIRCUIT HEAT MAPPER - ESP8266");
  Serial.println("  MLX90614 + SSD1306 OLED");
  Serial.println("========================================");

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW); // LED ON while starting

  // 1. Find the sensor and display buses, then initialize both.
  discoverBuses();

  // Wake any MLX90614 that might be in sleep mode. Skip the OLED bus so the
  // display's I2C state machine is not disturbed before it is initialized.
  for (uint8_t i = 0; i < BUS_COUNT; i++) {
    if (!busInfo[i].hasOled) {
      wakeBus(BUS_CONFIGS[i].sda, BUS_CONFIGS[i].scl);
    }
  }
  currentSda = -1;
  currentScl = -1;

  sensorOnline = initMlx();
  initOled();

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

    // Refresh the OLED so the IP appears even when the sensor is missing.
    if (oled.ready() && !sensorOnline) {
      char ipText[20];
      getIpText(ipText, sizeof(ipText));
      showMissingOnDisplay(ipText);
    }
  }

  uint32_t now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    if (sensorOnline) {
      ensureBus(sensorSda, sensorScl);
      float rawObj = (float)mlx.readObjectTempC();
      float rawAmb = (float)mlx.readAmbientTempC();

      if (!isValidReading(rawObj, rawAmb)) {
        // A failed read must not be fed into the DSP as 0 C.
        sensorOnline = false;
        digitalWrite(ONBOARD_LED, HIGH); // LED OFF
        webSocket.broadcastTXT("{\"online\":false}");
        Serial.println("[SENSOR] Read failed - entering retry mode.");
        showMissingOnDisplay("");
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
                 "{\"online\":true,\"t_obj\":%.2f,\"t_amb\":%.2f,\"delta\":%.2f,"
                 "\"rate\":%.2f,\"peak\":%.2f}",
                 m.filteredObj, m.rawAmb, m.delta, m.rateOfChange, m.peakTemp);
        webSocket.broadcastTXT(jsonBuffer);

        // Refresh the OLED at a lower rate; a full 128x64 frame takes ~90 ms
        // at 100 kHz. Bus switching adds ~60 ms when the display is separate.
        uint32_t displayInterval =
            displayOnSeparateBus ? DISPLAY_SEPARATE_MS : DISPLAY_INTERVAL_MS;
        if (oled.ready() && now - lastDisplayTime >= displayInterval) {
          lastDisplayTime = now;
          char ipText[20];
          getIpText(ipText, sizeof(ipText));
          showLiveOnDisplay(m, ipText);
        }

        // Periodic Serial Plotter trace
        static uint32_t lastSerial = 0;
        if (now - lastSerial >= 400) {
          lastSerial = now;
          Serial.printf("Obj: %.2f C | Amb: %.2f C | Delta: +%.2f C | Peak: %.2f C\n",
                        m.filteredObj, m.rawAmb, m.delta, m.peakTemp);
        }
      }
    } else {
      // Retry the MLX. If it has never been seen, scan every pin pair so a
      // sensor moved to another bus is picked up without a reboot.
      static uint32_t lastRetry = 0;
      if (now - lastRetry > SENSOR_RETRY_MS) {
        lastRetry = now;
        if (!mlxEverSeen) {
          scanForMlx();
        }
        sensorOnline = initMlx();
        if (sensorOnline) {
          webSocket.broadcastTXT("{\"online\":true}");
        } else if (oled.ready()) {
          char ipText[20];
          getIpText(ipText, sizeof(ipText));
          showMissingOnDisplay(ipText);
        }
      }

      // Keep dashboards informed without spamming the socket at 33 Hz.
      static uint32_t lastOfflineNotice = 0;
      if (!sensorOnline && now - lastOfflineNotice >= 1000) {
        lastOfflineNotice = now;
        webSocket.broadcastTXT("{\"online\":false}");
        Serial.println("[STATUS] Waiting for MLX90614 on the active I2C bus...");
      }
    }
  }
}
