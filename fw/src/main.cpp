#include <Arduino.h>
#include <ArduinoJson.h>

static bool ledOn = false;
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;

static void sendJson(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

static void setColor(uint8_t rr, uint8_t gg, uint8_t bb) {
  uint8_t rrr = (uint16_t)rr * brightness / 255;
  uint8_t ggg = (uint16_t)gg * brightness / 255;
  uint8_t bbb = (uint16_t)bb * brightness / 255;
  neopixelWrite(48, rrr, ggg, bbb);
  neopixelWrite(38, rrr, ggg, bbb);
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

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB Onboard";
  doc["version"] = "1.0";
  JsonArray mods = doc["modules"].to<JsonArray>();
  JsonObject m = mods.add<JsonObject>();
  m["id"] = "rgb";
  m["name"] = "Onboard RGB";
  JsonArray c = m["controls"].to<JsonArray>();

  JsonObject pw = c.add<JsonObject>();
  pw["id"] = "power"; pw["type"] = "switch"; pw["label"] = "Power"; pw["value"] = ledOn;

  JsonObject col = c.add<JsonObject>();
  col["id"] = "color"; col["type"] = "color"; col["label"] = "Color"; col["value"] = "#0000FF";

  JsonObject br = c.add<JsonObject>();
  br["id"] = "brightness"; br["type"] = "slider"; br["label"] = "Brightness";
  br["min"] = 0; br["max"] = 255; br["value"] = brightness;

  JsonObject btn = c.add<JsonObject>();
  btn["id"] = "rainbow"; btn["type"] = "button"; btn["label"] = "Rainbow Demo";

  sendJson(doc);
}

static void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char *cmd = doc["cmd"] | "";
  if (!strcmp(cmd, "describe")) { sendDescribe(); return; }

  if (!strcmp(cmd, "set")) {
    const char *id = doc["id"] | "";
    if (!strcmp(id, "power")) {
      ledOn = doc["value"].as<bool>();
      if (ledOn) setColor(r, g, b); else setColor(0, 0, 0);
      pushUpdateBool("power", ledOn);
    } else if (!strcmp(id, "color")) {
      String hex = String((const char *)(doc["value"] | "#0000FF"));
      if (hex.length() == 7 && hex[0] == '#') {
        r = strtol(hex.substring(1, 3).c_str(), nullptr, 16);
        g = strtol(hex.substring(3, 5).c_str(), nullptr, 16);
        b = strtol(hex.substring(5, 7).c_str(), nullptr, 16);
        if (ledOn) setColor(r, g, b);
        pushUpdateStr("color", hex);
      }
    } else if (!strcmp(id, "brightness")) {
      brightness = (uint8_t)doc["value"].as<int>();
      if (ledOn) setColor(r, g, b);
      pushUpdateInt("brightness", brightness);
    }
  } else if (!strcmp(cmd, "press")) {
    const char *id = doc["id"] | "";
    if (!strcmp(id, "rainbow")) {
      for (int i = 0; i < 360; i += 5) {
        int h = i;
        int x = 255 * (60 - abs(h % 120 - 60)) / 60;
        uint8_t rr, gg, bb;
        if (h < 60) { rr = 255; gg = x; bb = 0; }
        else if (h < 120) { rr = x; gg = 255; bb = 0; }
        else if (h < 180) { rr = 0; gg = 255; bb = x; }
        else if (h < 240) { rr = 0; gg = x; bb = 255; }
        else if (h < 300) { rr = x; gg = 0; bb = 255; }
        else { rr = 255; gg = 0; bb = x; }
        setColor(rr, gg, bb);
        delay(20);
      }
      if (ledOn) setColor(r, g, b); else setColor(0, 0, 0);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  setColor(0, 0, 0);
  sendDescribe();
}

void loop() {
  static String lineBuf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { handleLine(lineBuf); lineBuf = ""; }
    else if (c != '\r' && lineBuf.length() < 512) lineBuf += c;
  }
}