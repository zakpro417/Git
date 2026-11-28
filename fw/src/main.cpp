#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#define PIN_RGB 48
#define NUM_PIXELS 1

Preferences prefs;
Adafruit_NeoPixel strip(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);
WiFiServer server(80);

static bool ledOn = false;
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;
static String storedSsid = "";
static String storedPass = "";

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
}

static String wifiStatusStr() {
  String s;
  if (WiFi.status() == WL_CONNECTED) {
    s = "WiFi: " + WiFi.localIP().toString() + " | AP: " + WiFi.softAPIP().toString();
  } else {
    s = "AP: " + WiFi.softAPIP().toString() + " (not connected)";
  }
  return s;
}

static void startAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP-RGB", "12345678");
  Serial.printf("AP started IP: %s\n", WiFi.softAPIP().toString().c_str());
}

static bool tryConnect(const String &ssid, const String &pass) {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("Connecting to %s", ssid.c_str());
  int w = 0;
  while (WiFi.status() != WL_CONNECTED && w < 20) {
    delay(500);
    Serial.print(".");
    w++;
  }
  bool ok = (WiFi.status() == WL_CONNECTED);
  if (ok) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connect failed");
  }
  return ok;
}

static void scanWiFi() {
  pushUpdateStr("wifi_status", "Scanning...");
  int n = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", n);
  String list;
  if (n <= 0) {
    list = "(none found)";
  } else {
    if (n > 10) n = 10;
    for (int i = 0; i < n; i++) {
      if (i > 0) list += ", ";
      list += WiFi.SSID(i);
      list += " (";
      list += String(WiFi.RSSI(i));
      list += "dBm)";
    }
  }
  WiFi.scanDelete();
  pushUpdateStr("networks", list);
  pushUpdateStr("wifi_status", wifiStatusStr());
  Serial.println("Networks: " + list);
}

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB WiFi";
  doc["version"] = "3.0";
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
  JsonObject scan = c.add<JsonObject>();
  scan["id"] = "scan_wifi"; scan["type"] = "button"; scan["label"] = "Scan WiFi";
  JsonObject nets = c.add<JsonObject>();
  nets["id"] = "networks"; nets["type"] = "readout"; nets["label"] = "Networks"; nets["value"] = "(press Scan WiFi)";
  JsonObject ssid = c.add<JsonObject>();
  ssid["id"] = "ssid"; ssid["type"] = "text"; ssid["label"] = "WiFi SSID"; ssid["value"] = storedSsid;
  JsonObject pass = c.add<JsonObject>();
  pass["id"] = "pass"; pass["type"] = "text"; pass["label"] = "WiFi Password"; pass["value"] = "";
  JsonObject conn = c.add<JsonObject>();
  conn["id"] = "connect_wifi"; conn["type"] = "button"; conn["label"] = "Connect WiFi";
  JsonObject wf = c.add<JsonObject>();
  wf["id"] = "wifi_status"; wf["type"] = "readout"; wf["label"] = "WiFi Status"; wf["value"] = wifiStatusStr();
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
    } else if (!strcmp(id, "ssid")) {
      storedSsid = doc["value"].as<String>();
    } else if (!strcmp(id, "pass")) {
      storedPass = doc["value"].as<String>();
    }
  } else if (!strcmp(cmd, "press")) {
    const char* id = doc["id"] | "";
    if (!strcmp(id, "rainbow")) {
      for (int x = 0; x < 256; x++) {
        if (x < 85) { r = x * 3; g = 255 - x * 3; b = 0; }
        else if (x < 170) { int t = x - 85; r = 255 - t * 3; g = 0; b = t * 3; }
        else { int t = x - 170; r = 0; g = t * 3; b = 255 - t * 3; }
        setColor(r, g, b);
        delay(20);
      }
      setColor(r, g, b);
    } else if (!strcmp(id, "scan_wifi")) {
      scanWiFi();
    } else if (!strcmp(id, "connect_wifi")) {
      if (storedSsid.length() > 0) {
        pushUpdateStr("wifi_status", "Connecting...");
        bool ok = tryConnect(storedSsid, storedPass);
        if (ok) {
          saveWiFiConfig();
        }
        pushUpdateStr("wifi_status", wifiStatusStr());
      } else {
        pushUpdateStr("wifi_status", "Enter SSID first!");
      }
    }
  }
}

static const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>ESP WiFi</title></head><body><h2>WiFi setup</h2><p><a href='/scan'>Scan networks</a></p><div id='n'></div><form action='/wifi_save' method='GET'>
SSID: <input type='text' name='ssid' value='' required><br>Password: <input type='password' name='pass'><br><br><button type='submit'>Save & Connect</button></form></body></html>
)rawliteral";

static void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;
  String req = client.readStringUntil('\r');
  client.flush();
  if (req.indexOf("GET /scan") >= 0) {
    int n = WiFi.scanNetworks();
    String body = "Found networks:\n";
    for (int i = 0; i < n && i < 15; i++) {
      body += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)\n";
    }
    WiFi.scanDelete();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print(body);
    client.stop();
    return;
  }
  if (req.indexOf("GET /wifi_save") >= 0) {
    int qs = req.indexOf('?');
    if (qs >= 0) {
      String query = req.substring(qs + 1);
      int ssIdx = query.indexOf("ssid=");
      int passIdx = query.indexOf("pass=");
      String s = (ssIdx >= 0) ? query.substring(ssIdx + 5) : "";
      String p = (passIdx >= 0) ? query.substring(passIdx + 5) : "";
      int amp = s.indexOf('&');
      if (amp >= 0) s = s.substring(0, amp);
      if (s.length() > 0) {
        storedSsid = s;
        storedPass = p;
        saveWiFiConfig();
        tryConnect(storedSsid, storedPass);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println(wifiStatusStr());
        client.stop();
        return;
      }
    }
    client.stop();
    return;
  }
  if (req.indexOf("GET /") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(htmlPage);
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
  startAP();
  if (storedSsid.length() > 0) {
    tryConnect(storedSsid, storedPass);
  }
  server.begin();
  sendDescribe();
  pushUpdateStr("wifi_status", wifiStatusStr());
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