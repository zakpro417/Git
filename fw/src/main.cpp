#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#define PIN_RGB 48
#define NUM_PIXELS 1

Preferences prefs;
Adafruit_NeoPixel strip(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

static bool ledOn = false;
static bool apOn = false;
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;
static String storedSsid = "";
static String storedPass = "";

static void saveWiFiConfig() {
  prefs.begin("wifi", true);
  prefs.putString("ssid", storedSsid);
  prefs.putString("pass", storedPass);
  prefs.end();
}

static void loadWiFiConfig() {
  prefs.begin("wifi", false);
  storedSsid = prefs.getString("ssid", "");
  storedPass = prefs.getString("pass", "");
  prefs.end();
  Serial.printf("Loaded WiFi: %s\n", storedSsid.c_str());
}

static void connectWiFi() {
  if (storedSsid.length() == 0) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(storedSsid.c_str(), storedPass.c_str());
  int w = 0;
  while (WiFi.status() != WL_CONNECTED && w < 20) {
    delay(500);
    Serial.print(".");
    w++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    apOn = false;
    WiFi.softAPdisconnect(true);
  } else {
    Serial.println("\nWiFi connect failed, starting AP mode");
    startAP();
  }
}

static void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP-RGB", "12345678");
  apOn = true;
  Serial.printf("AP started IP: %s\n", WiFi.softAPIP().toString().c_str());
}

static void sendJson(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

static void pushUpdateStr(const char *id, const String &val) {
  JsonDocument d;
  d["type"] = "update";
  d["module"] = "rgb";
  d["id"] = id;
  d["value"] = val;
  sendJson(d);
}

static void pushUpdateBool(const char *id, bool val) {
  JsonDocument d;
  d["type"] = "update";
  d["module"] = "rgb";
  d["id"] = id;
  d["value"] = val;
  sendJson(d);
}

static void pushUpdateInt(const char *id, int val) {
  JsonDocument d;
  d["type"] = "update";
  d["module"] = "rgb";
  d["id"] = id;
  d["value"] = val;
  sendJson(d);
}

static void setColor(uint8_t rr, uint8_t gg, uint8_t bb) {
  if (!ledOn) { rr = 0; gg = 0; bb = 0; }
  uint16_t valR = (uint16_t)rr * brightness / 255;
  uint16_t valG = (uint16_t)gg * brightness / 255;
  uint16_t valB = (uint16_t)bb * brightness / 255;
  strip.setPixelColor(0, valR, valG, valB);
  strip.show();
}

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB WiFi";
  doc["version"] = "1.0";
  JsonArray mods = doc["modules"].to<JsonArray>();
  JsonObject m = mods.add<JsonObject>();
  m["id"] = "rgb";
  m["name"] = "RGB WiFi";
  JsonArray c = m["controls"].to<JsonArray>();
  JsonObject pw = c.add<JsonObject>();
  pw["id"] = "power"; pw["type"] = "switch"; pw["label"] = "Power"; pw["value"] = ledOn;
  JsonObject col = c.add<JsonObject>();
  col["id"] = "color"; col["type"] = "color"; col["label"] = "Color"; col["value"] = "#0000FF";
  JsonObject bri = c.add<JsonObject>();
  bri["id"] = "brightness"; bri["type"] = "slider"; bri["label"] = "Brightness"; bri["min"] = 0; bri["max"] = 255; bri["value"] = brightness;
  JsonObject rain = c.add<JsonObject>();
  rain["id"] = "rainbow"; rain["type"] = "button"; rain["label"] = "Rainbow Demo";
  // add wifi control placeholder
  JsonObject wf = c.add<JsonObject>();
  wf["id"] = "wifi"; wf["type"] = "readout"; wf["label"] = "WiFi"; wf["value"] = WiFi.localIP().toString();
  sendJson(doc);
}

static void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char* cmd = doc["cmd"] | "";
  if (!strcmp(cmd, "describe")) { sendDescribe(); return; }
  if (!strcmp(cmd, "set")) {
    const char* id = doc["id"] | "";
    if (!strcmp(id, "power")) {
      ledOn = doc["value"].as<bool>();
      setColor(r, g, b);
      pushUpdateBool("power", ledOn);
    } else if (!strcmp(id, "color")) {
      const char* hex = doc["value"] | "#000000";
      if (strlen(hex) == 7) {
        unsigned long val = strtoul(hex + 1, NULL, 16);
        r = (val >> 16) & 0xFF;
        g = (val >> 8) & 0xFF;
        b = val & 0xFF;
      }
      setColor(r, g, b);
      pushUpdateStr("color", hex);
    } else if (!strcmp(id, "brightness")) {
      brightness = doc["value"].as<int>();
      setColor(r, g, b);
      pushUpdateInt("brightness", brightness);
    } else if (!strcmp(id, "wifi")) {
      // expect value object with ssid and pass
      const char* ssid = doc["value"]["ssid"] | "";
      const char* pass = doc["value"]["pass"] | "";
      if (ssid.length() > 0 && pass.length() > 0) {
        storedSsid = ssid;
        storedPass = pass;
        saveWiFiConfig();
        ESP.restart();
      }
    }
  } else if (!strcmp(cmd, "press")) {
    const char* id = doc["id"] | "";
    if (!strcmp(id, "rainbow")) {
      for (int x = 0; x < 256; x++) {
        if (x < 85) { r = x * 3; g = 255 - x * 3; b = 0; }
        else if (x < 170) { x -= 85; r = 255 - x * 3; g = 0; b = x * 3; }
        else { x -= 170; r = 0; g = x * 3; b = 255 - x * 3; }
        setColor(r, g, b);
        delay(20);
      }
      setColor(r, g, b);
    }
  }
}

// Simple HTML page for WiFi config
static const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>ESP WiFi Config</title></head><body><h2>Connect to WiFi</h2><form action='/wifi_save' method='GET'>
SSID: <input type='text' name='ssid' required><br>Password: <input type='password' name='pass' required><br><br><button type='submit'>Save & Connect</button></form><p>Current IP: <span id='ip'>%IP%</span></p></body></html>
)rawliteral";

static void sendHtmlPage(WiFiClient &client) {
  String ip = WiFi.localIP().toString();
  String page = htmlPage;
  page.replace("%IP%", ip);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.print(page);
}

static void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;
  String req = client.readStringUntil('\r');
  client.flush();
  if (req.indexOf("GET /") < 0) { client.stop(); return; }
  if (req.indexOf("GET /wifi_save") >= 0) {
    int qs = req.indexOf('?');
    if (qs >= 0) {
      String query = req.substring(qs + 1);
      int ssIdx = query.indexOf("ssid=");
      int passIdx = query.indexOf("pass=");
      String ssid = (ssIdx >= 0) ? query.substring(ssIdx + 5) : "";
      String pass = (passIdx >= 0) ? query.substring(passIdx + 5) : "";
      if (ssid.length() > 0 && pass.length() > 0) {
        storedSsid = ssid;
        storedPass = pass;
        saveWiFiConfig();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Saving... restarting");
        delay(500);
        ESP.restart();
      }
    }
    client.stop();
    return;
  }
  if (req.indexOf("GET /") >= 0) {
    sendHtmlPage(client);
    client.stop();
    return;
  }
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  strip.begin();
  strip.setBrightness(255);
  strip.show();
  setColor(0, 0, 0);
  loadWiFiConfig();
  connectWiFi();
  startAP(); // ensure AP always available for config
  server.begin();
  sendDescribe();
}

void loop() {
  handleWebServer();
  static String lineBuf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { handleLine(lineBuf); lineBuf = ""; }
    else if (c != '\r' && lineBuf.length() < 512) lineBuf += c;
  }
}