#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RGB 48
#define NUM_PIXELS 1

Adafruit_NeoPixel strip = Adafruit_New(PIN_RGB, NUM_PIXELS, NEOPIXEL_BLACK);

static bool ledOn = false;
static uint8_t r = 0, g = 0, b = 255;
static uint8_t brightness = 128;

static JsonDocument doc;
static String lineBuf;

static void sendJson(JsonDocument &docIn) {
  serializeJson(docIn, Serial);
  Serial.println();
}

static void setColor(uint8_t rr, uint8_t gg, uint8_t bb) {
  uint16_t valR = (uint16_t)rr * brightness / 255;
  uint16_t valG = (uint16_t)gg * brightness / 255;
  uint16_t valB = (uint16_t)bb * brightness / 255;
  strip.setPixelColor(valR, valG, valB);
  strip.show();
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
  m["name"] = "RGB Strip";
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

void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char* cmd = doc["cmd"].as<string>();

  if (strcmp(cmd, "describe")) { sendDescribe(); return; }
  if (strcmp(cmd, "set") && !strcmp(doc["id"].as<string>(), "ledOn")) {
    ledOn = doc["value"].as<bool>();
  } else if (strcmp(cmd, "set") && !strcmp(doc["id"].as<string>(), "power")) {
    apOn = doc["value"].as<bool>(); // placeholder
  }
  // Additional fields can be parsed similarly
}

void setup() {
  Serial.begin(115200);
  delay(500);
  sendDescribe();
}

void loop() {
  static String lineBuf;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handleLine(lineBuf);
      lineBuf = "";
    } else if (c != '\r' && lineBuf.length() < 512) {
      lineBuf += c;
    }
  }
}