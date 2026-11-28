#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#define PIN_RGB 48
#define NUM_PIXELS 1
#define MAX_NETS 10

Preferences prefs;
Adafruit_NeoPixel strip(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

static bool ledOn = false;
static uint8_t rC = 0, gC = 0, bC = 255;
static uint8_t bright = 128;
static String storedSsid = "";
static String storedPass = "";
static String selectedSsid = "";
static String passVal = "";
static String netSsid[MAX_NETS];
static int netRssi[MAX_NETS];
static int netCount = 0;
static unsigned long rainbowUntil = 0;
static String lineBuf;

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

static void sendDescribe();

static void applyLed() {
  if (ledOn) strip.setPixelColor(0, strip.Color(rC, gC, bC));
  else strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.setBrightness(bright);
  strip.show();
}

static void connectWiFi(const String &ssid, const String &pass) {
  if (ssid.length() == 0) {
    pushUpdateStr("wifi_status", "Select a network first!");
    return;
  }
  pushUpdateStr("wifi_status", "Connecting to " + ssid + " ...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(200);
  if (WiFi.status() == WL_CONNECTED) {
    pushUpdateStr("wifi_status", "Connected: " + WiFi.localIP().toString());
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    storedSsid = ssid;
    storedPass = pass;
  } else {
    pushUpdateStr("wifi_status", "Failed to connect to " + ssid);
  }
}

static void scanWiFi() {
  pushUpdateStr("networks", "Scanning...");
  WiFi.mode(WIFI_AP_STA);
  netCount = WiFi.scanNetworks();
  if (netCount < 0) netCount = 0;
  if (netCount > MAX_NETS) netCount = MAX_NETS;
  String list = "";
  for (int i = 0; i < netCount; i++) {
    netSsid[i] = WiFi.SSID(i);
    netRssi[i] = WiFi.RSSI(i);
    list += netSsid[i] + " (" + String(netRssi[i]) + "dBm)";
    if (i < netCount - 1) list += ", ";
  }
  WiFi.scanDelete();
  if (netCount == 0) list = "No networks found";
  pushUpdateStr("networks", list);
  sendDescribe();
}

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB WiFi";
  doc["version"] = "5.0";
  JsonObject mod = doc["modules"].to<JsonArray>().add<JsonObject>();
  mod["id"] = "rgb";
  mod["name"] = "RGB WiFi";
  JsonArray ctl = mod["controls"].to<JsonArray>();
  JsonObject sw = ctl.add<JsonObject>();
  sw["id"] = "power"; sw["type"] = "switch"; sw["label"] = "Power"; sw["value"] = ledOn;
  JsonObject col = ctl.add<JsonObject>();
  col["id"] = "color"; col["type"] = "color"; col["label"] = "Color";
  char hex[8];
  snprintf(hex, sizeof(hex), "#%02X%02X%02X", rC, gC, bC);
  col["value"] = hex;
  JsonObject br = ctl.add<JsonObject>();
  br["id"] = "brightness"; br["type"] = "slider"; br["label"] = "Brightness";
  br["min"] = 0; br["max"] = 255; br["value"] = bright;
  JsonObject rb = ctl.add<JsonObject>();
  rb["id"] = "rainbow"; rb["type"] = "button"; rb["label"] = "Rainbow Demo";
  JsonObject sc = ctl.add<JsonObject>();
  sc["id"] = "scan_wifi"; sc["type"] = "button"; sc["label"] = "Scan WiFi";
  for (int i = 0; i < netCount; i++) {
    JsonObject nb = ctl.add<JsonObject>();
    nb["id"] = String("net_") + i;
    nb["type"] = "button";
    nb["label"] = netSsid[i] + " (" + String(netRssi[i]) + " dBm)";
  }
  JsonObject nt = ctl.add<JsonObject>();
  nt["id"] = "networks"; nt["type"] = "readout"; nt["label"] = "Networks";
  nt["value"] = netCount > 0 ? "Tap a network above" : "(press Scan WiFi)";
  JsonObject pw = ctl.add<JsonObject>();
  pw["id"] = "pass"; pw["type"] = "text"; pw["label"] = "WiFi Password"; pw["value"] = passVal;
  JsonObject cn = ctl.add<JsonObject>();
  cn["id"] = "connect_wifi"; cn["type"] = "button"; cn["label"] = "Connect WiFi";
  JsonObject st = ctl.add<JsonObject>();
  st["id"] = "wifi_status"; st["type"] = "readout"; st["label"] = "WiFi Status";
  st["value"] = WiFi.status() == WL_CONNECTED ? ("Connected: " + WiFi.localIP().toString()) : "Not connected";
  sendJson(doc);
}

static void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char *cmd = doc["cmd"] | "";
  const char *id = doc["id"] | "";
  if (!strcmp(cmd, "describe")) { sendDescribe(); return; }
  if (!strcmp(cmd, "set")) {
    if (!strcmp(id, "power")) { ledOn = doc["value"].as<bool>(); applyLed(); pushUpdateBool("power", ledOn); }
    else if (!strcmp(id, "brightness")) { bright = doc["value"].as<uint8_t>(); applyLed(); }
    else if (!strcmp(id, "color")) {
      String h = doc["value"].as<String>();
      if (h.length() == 7 && h[0] == '#') {
        rC = strtol(h.substring(1, 3).c_str(), NULL, 16);
        gC = strtol(h.substring(3, 5).c_str(), NULL, 16);
        bC = strtol(h.substring(5, 7).c_str(), NULL, 16);
        applyLed();
      }
    }
    else if (!strcmp(id, "pass")) { passVal = doc["value"].as<String>(); }
    else if (!strcmp(id, "network")) { selectedSsid = doc["value"].as<String>(); }
    return;
  }
  if (!strcmp(cmd, "press")) {
    if (!strcmp(id, "rainbow")) { rainbowUntil = millis() + 5000; }
    else if (!strcmp(id, "scan_wifi")) { scanWiFi(); }
    else if (!strcmp(id, "connect_wifi")) { connectWiFi(selectedSsid, passVal); }
    else if (strncmp(id, "net_", 4) == 0) {
      int i = atoi(id + 4);
      if (i >= 0 && i < netCount) {
        selectedSsid = netSsid[i];
        pushUpdateStr("wifi_status", "Selected: " + selectedSsid + " — enter password, then Connect");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(bright);
  strip.show();
  prefs.begin("wifi", false);
  storedSsid = prefs.getString("ssid", "");
  storedPass = prefs.getString("pass", "");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-Setup");
  Serial.println("AP started IP: " + WiFi.softAPIP().toString());
  if (storedSsid.length() > 0) {
    WiFi.begin(storedSsid.c_str(), storedPass.c_str());
  }
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { handleLine(lineBuf); lineBuf = ""; }
    else if (c != '\r' && lineBuf.length() < 512) lineBuf += c;
  }
  if (rainbowUntil > millis()) {
    uint8_t pos = (millis() / 20) & 255;
    strip.setPixelColor(0, strip.ColorHSV(pos * 256));
    strip.setBrightness(bright);
    strip.show();
  } else if (rainbowUntil != 0 && millis() >= rainbowUntil) {
    rainbowUntil = 0;
    applyLed();
  }
  static unsigned long lastConn = 0;
  if (millis() - lastConn > 5000) {
    lastConn = millis();
    if (WiFi.status() == WL_CONNECTED) pushUpdateStr("wifi_status", "Connected: " + WiFi.localIP().toString());
  }
}\n