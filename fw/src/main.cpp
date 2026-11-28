#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RGB 48
#define NUM_PIXELS 1

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);
WiFiServer server(80);

static bool ledOn = false;
static bool apOn = false;
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;

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

static void startAP() {
  WiFi.softAP("ESP-RGB", "12345678");
  apOn = true;
  server.begin();
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

static void stopAP() {
  WiFi.softAPdisconnect(true);
  apOn = false;
}

static int getParam(const String &query, const char *name, int def) {
  String key = String(name) + "=";
  int idx = query.indexOf(key);
  if (idx < 0) return def;
  int start = idx + key.length();
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();
  String val = query.substring(start, end);
  return val.toInt();
}

static void handleWiFi() {
  WiFiClient client = server.available();
  if (!client) return;
  String req = client.readStringUntil('\r');
  client.flush();
  if (req.indexOf("GET /") < 0) { client.stop(); return; }
  if (req.indexOf("GET /set") >= 0) {
    int qs = req.indexOf('?');
    if (qs >= 0) {
      String query = req.substring(qs + 1);
      query.trim();
      int idx = 0;
      while (idx < query.length()) {
        int amp = query.indexOf('&', idx);
        String pair = (amp < 0) ? query.substring(idx) : query.substring(idx, amp);
        int eq = pair.indexOf('=');
        if (eq >= 0) {
          String name = pair.substring(0, eq);
          String val = pair.substring(eq + 1);
          if (name == "r") r = val.toInt();
          else if (name == "g") g = val.toInt();
          else if (name == "b") b = val.toInt();
          else if (name == "power") ledOn = (val == "1" || val == "on");
          else if (name == "brightness") brightness = val.toInt();
        }
        idx = (amp < 0) ? query.length() : amp + 1;
      }
      setColor(r, g, b);
    }
  } else if (req.indexOf("GET /power") >= 0) {
    int qs = req.indexOf('?');
    if (qs >= 0) {
      String query = req.substring(qs + 1);
      int eq = query.indexOf('=');
      if (eq >= 0) {
        String val = query.substring(eq + 1);
        ledOn = (val == "1" || val == "on");
        setColor(r, g, b);
      }
    }
  }
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  strip.begin();
  strip.setBrightness(255);
  strip.show();
  setColor(0, 0, 0);
  startAP();
  sendDescribe();
}

void loop() {
  handleWiFi();
  static String lineBuf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { handleLine(lineBuf); lineBuf = ""; }
    else if (c != '\r' && lineBuf.length() < 512) lineBuf += c;
  }
}