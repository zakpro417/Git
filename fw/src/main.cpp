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
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;
static String storedSsid = "";
static String storedPass = "";
static String selectedSsid = "";
static String netSsid[MAX_NETS];
static int netRssi[MAX_NETS];
static int netEnc[MAX_NETS];
static int netCount = 0;
static unsigned long rainbowUntil = 0;

static void sendJson(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

static void sendDescribe();

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

static String colorHex() {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
  return String(buf);
}

static void hexToRgb(const String &hex) {
  if (hex.length() >= 7 && hex[0] == '#') {
    r = strtol(hex.substring(1, 3).c_str(), NULL, 16);
    g = strtol(hex.substring(3, 5).c_str(), NULL, 16);
    b = strtol(hex.substring(5, 7).c_str(), NULL, 16);
  }
}

static void applyColor() {
  strip.setBrightness(brightness);
  if (ledOn) strip.setPixelColor(0, r, g, b);
  else strip.setPixelColor(0, 0, 0, 0);
  strip.show();
}

static void saveWiFiConfig() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", storedSsid);
  prefs.putString("pass", storedPass);
  prefs.end();
}

static void loadWiFiConfig() {
  prefs.begin("wifi", true);
  storedSsid = prefs.getString("ssid", "");
  storedPass = prefs.getString("pass", "");
  prefs.end();
}

static String statusStr() {
  if (WiFi.status() == WL_CONNECTED) return "Connected: " + WiFi.localIP().toString();
  return "AP: 192.168.4.1 (not connected)";
}

static void startAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP-RGB");
  Serial.println("AP started IP: 192.168.4.1");
}

static bool connectWiFi(const String &ssid, const String &pass) {
  pushUpdateStr("wifi_status", "Connecting to " + ssid + " ...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(200);
  if (WiFi.status() == WL_CONNECTED) {
    storedSsid = ssid;
    storedPass = pass;
    saveWiFiConfig();
    pushUpdateStr("wifi_status", statusStr());
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    return true;
  }
  pushUpdateStr("wifi_status", "Failed: " + ssid);
  Serial.println("WiFi connect failed: " + ssid);
  return false;
}

static void scanWiFi() {
  pushUpdateStr("networks", "Scanning...");
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  if (n < 0) n = 0;
  netCount = n > MAX_NETS ? MAX_NETS : n;
  for (int i = 0; i < netCount; i++) {
    netSsid[i] = WiFi.SSID(i);
    netRssi[i] = WiFi.RSSI(i);
    netEnc[i] = WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
  String list = "";
  for (int i = 0; i < netCount; i++) {
    if (i) list += ", ";
    list += netSsid[i] + " (" + String(netRssi[i]) + "dBm)";
  }
  if (netCount == 0) list = "No networks found";
  pushUpdateStr("networks", list);
  Serial.println("Scan: " + String(netCount) + " networks");
  sendDescribe();
}

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB WiFi";
  doc["version"] = "4.0";
  JsonObject mod = doc["modules"].to<JsonArray>().add<JsonObject>();
  mod["id"] = "rgb";
  mod["name"] = "RGB WiFi";
  JsonArray ctl = mod["controls"].to<JsonArray>();

  JsonObject sw = ctl.add<JsonObject>();
  sw["id"] = "power"; sw["type"] = "switch"; sw["label"] = "Power"; sw["value"] = ledOn;

  JsonObject col = ctl.add<JsonObject>();
  col["id"] = "color"; col["type"] = "color"; col["label"] = "Color"; col["value"] = colorHex();

  JsonObject br = ctl.add<JsonObject>();
  br["id"] = "brightness"; br["type"] = "slider"; br["label"] = "Brightness";
  br["min"] = 0; br["max"] = 255; br["value"] = (int)brightness;

  JsonObject rb = ctl.add<JsonObject>();
  rb["id"] = "rainbow"; rb["type"] = "button"; rb["label"] = "Rainbow Demo";

  JsonObject sc = ctl.add<JsonObject>();
  sc["id"] = "scan_wifi"; sc["type"] = "button"; sc["label"] = "Scan WiFi";

  JsonObject net = ctl.add<JsonObject>();
  net["id"] = "network"; net["type"] = "select"; net["label"] = "WiFi Network";
  net["value"] = selectedSsid;
  JsonArray opts = net["options"].to<JsonArray>();
  for (int i = 0; i < netCount; i++) opts.add(netSsid[i]);

  for (int i = 0; i < netCount; i++) {
    JsonObject nb = ctl.add<JsonObject>();
    nb["id"] = String("wifi_") + String(i);
    nb["type"] = "button";
    nb["label"] = netSsid[i] + " (" + String(netRssi[i]) + "dBm)";
  }

  JsonObject pass = ctl.add<JsonObject>();
  pass["id"] = "pass"; pass["type"] = "text"; pass["label"] = "WiFi Password"; pass["value"] = "";

  JsonObject cn = ctl.add<JsonObject>();
  cn["id"] = "connect_wifi"; cn["type"] = "button"; cn["label"] = "Connect WiFi";

  JsonObject st = ctl.add<JsonObject>();
  st["id"] = "wifi_status"; st["type"] = "readout"; st["label"] = "WiFi Status"; st["value"] = statusStr();

  JsonObject lst = ctl.add<JsonObject>();
  lst["id"] = "networks"; lst["type"] = "readout"; lst["label"] = "Networks";
  lst["value"] = netCount > 0 ? String("Tap a network above") : String("(press Scan WiFi)");

  sendJson(doc);
}

static void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char *cmd = doc["cmd"] | "";
  const char *id = doc["id"] | "";
  if (!strcmp(cmd, "describe")) { sendDescribe(); return; }
  if (!strcmp(cmd, "set")) {
    if (!strcmp(id, "power")) { ledOn = doc["value"].as<bool>(); applyColor(); pushUpdateBool("power", ledOn); }
    else if (!strcmp(id, "color")) { hexToRgb(doc["value"].as<String>()); applyColor(); pushUpdateStr("color", colorHex()); }
    else if (!strcmp(id, "brightness")) { brightness = doc["value"].as<int>(); applyColor(); pushUpdateInt("brightness", brightness); }
    else if (!strcmp(id, "network")) { selectedSsid = doc["value"].as<String>(); pushUpdateStr("wifi_status", "Selected: " + selectedSsid); }
    else if (!strcmp(id, "pass")) { storedPass = doc["value"].as<String>(); }
    return;
  }
  if (!strcmp(cmd, "press")) {
    if (!strcmp(id, "rainbow")) { rainbowUntil = millis() + 10000; }
    else if (!strcmp(id, "scan_wifi")) { scanWiFi(); }
    else if (!strcmp(id, "connect_wifi")) {
      String s = selectedSsid.length() > 0 ? selectedSsid : storedSsid;
      if (s.length() == 0) { pushUpdateStr("wifi_status", "No network selected"); return; }
      connectWiFi(s, storedPass);
    }
    else if (strncmp(id, "wifi_", 5) == 0) {
      int idx = String(id + 5).toInt();
      if (idx >= 0 && idx < netCount) {
        selectedSsid = netSsid[idx];
        pushUpdateStr("network", selectedSsid);
        pushUpdateStr("wifi_status", "Selected: " + selectedSsid + " (enter password, then Connect)");
        Serial.println("Selected network: " + selectedSsid);
        if (netEnc[idx] == WIFI_AUTH_OPEN) connectWiFi(selectedSsid, "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  strip.begin();
  strip.setBrightness(255);
  strip.show();
  strip.setPixelColor(0, 0, 0, 0);
  strip.show();
  loadWiFiConfig();
  startAP();
  if (storedSsid.length() > 0) connectWiFi(storedSsid, storedPass);
  sendDescribe();
}

void loop() {
  static String lineBuf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { handleLine(lineBuf); lineBuf = ""; }
    else if (c != '\r' && lineBuf.length() < 512) lineBuf += c;
  }
  if (rainbowUntil > millis()) {
    strip.setBrightness(brightness);
    strip.setPixelColor(0, strip.ColorHSV((uint16_t)((millis() / 5) % 65536)));
    strip.show();
  } else if (rainbowUntil != 0) {
    rainbowUntil = 0;
    applyColor();
  }
}