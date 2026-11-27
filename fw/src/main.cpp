#include <Arduino.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#define LED_PIN    46
#define NUM_LEDS   1
#define BRIGHTNESS 60

CRGB leds[NUM_LEDS];

// Палитра: плавный перебор цветов
const CRGBPalette16 palette = CRGBPalette16(
  CRGB::Red, CRGB::OrangeRed, CRGB::Gold, CRGB::Yellow,
  CRGB::Green, CRGB::Aquamarine, CRGB::Cyan, CRGB::Blue,
  CRGB::Blue, CRGB::BlueViolet, CRGB::Magenta, CRGB::Pink,
  CRGB::Red, CRGB::OrangeRed, CRGB::Gold, CRGB::Yellow);

bool  gPower = true;
float gPhase = 0.0;
uint32_t gLast = 0;
int gSpeed = 6;
CRGB gFixed = CRGB::Red;
bool gUseFixed = false;

void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB Blink ESP32-S3";
  doc["version"] = "1.1";

  JsonArray mods = doc["modules"].to<JsonArray>();

  JsonObject m = mods.add<JsonObject>();
  m["id"] = "rgb";
  m["name"] = "RGB LED";

  JsonArray c = m["controls"].to<JsonArray>();

  JsonObject o = c.add<JsonObject>();
  o["id"] = "power"; o["type"] = "switch"; o["label"] = "Power"; o["value"] = gPower;

  o = c.add<JsonObject>();
  o["id"] = "speed"; o["type"] = "slider"; o["label"] = "Speed";
  o["min"] = 1; o["max"] = 20; o["value"] = gSpeed;

  o = c.add<JsonObject>();
  o["id"] = "bright"; o["type"] = "slider"; o["label"] = "Brightness";
  o["min"] = 0; o["max"] = 255; o["value"] = BRIGHTNESS;

  o = c.add<JsonObject>();
  o["id"] = "color"; o["type"] = "color"; o["label"] = "Color"; o["value"] = "#FF0000";

  o = c.add<JsonObject>();
  o["id"] = "reset"; o["type"] = "button"; o["label"] = "Reset";

  String out;
  serializeJson(doc, out);
  Serial.println(out);
}

void applySet(JsonObject root) {
  String id = root["id"] | "";
  JsonVariant v = root["value"];
  if (id == "power") gPower = v.as<bool>();
  else if (id == "speed") gSpeed = v.as<int>();
  else if (id == "bright") FastLED.setBrightness(v.as<int>());
  else if (id == "color") {
    String s = v.as<String>();
    if (s.length() == 7 && s[0] == '#') {
      gFixed = CRGB(strtol(s.substring(1,3).c_str(), nullptr, 16),
                    strtol(s.substring(3,5).c_str(), nullptr, 16),
                    strtol(s.substring(5,7).c_str(), nullptr, 16));
      gUseFixed = true;
    }
  }
}

void doReset() {
  gPower = true;
  gSpeed = 6;
  gUseFixed = false;
  gPhase = 0.0;
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("{\"type\":\"update\",\"module\":\"rgb\",\"id\":\"power\",\"value\":true}");
  Serial.println("{\"type\":\"update\",\"module\":\"rgb\",\"id\":\"speed\",\"value\":6}");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  gLast = millis();
  sendDescribe();
  Serial.println("RGB blink ready");
}

void loop() {
  // приём команд
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("{")) continue;
    JsonDocument doc;
    if (deserializeJson(doc, line)) continue;
    String cmd = doc["cmd"] | "";
    if (cmd == "describe") sendDescribe();
    else if (cmd == "set") applySet(doc.as<JsonObject>());
    else if (cmd == "press") {
      String id = doc["id"] | "";
      if (id == "reset") doReset();
    }
  }

  uint32_t now = millis();
  if (gPower && now - gLast > 30) {
    gLast = now;
    if (gUseFixed) {
      // статичный выбранный цвет с лёгким "дыханием"
      float b = 0.5f + 0.5f * sin(gPhase);
      leds[0] = gFixed;
      leds[0].nscale8(120 + 135 * b);
      gPhase += 0.05f;
    } else {
      gPhase += 0.02f * gSpeed / 6.0f;
      leds[0] = ColorFromPalette(palette, (uint8_t)(gPhase * 16), 255, LINEARBLEND);
    }
    FastLED.show();
  } else if (!gPower) {
    leds[0] = CRGB::Black;
    FastLED.show();
  }
}