#include <Arduino.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#define LED_PIN    48
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

void sendDescribe() {
  StaticJsonDocument<1024> doc;
  doc["type"] = "describe";
  doc["firmware"] = "RGB Blink ESP32-S3";
  doc["version"] = "1.0";

  JsonObject mod = doc.createNestedObject("modules");
  mod.createNestedObject("id"); // placeholder removed below
  doc["modules"].remove(0);

  JsonObject m = doc["modules"].createNestedObject();
  m["id"] = "rgb";
  m["name"] = "RGB LED";

  JsonArray c = m.createNestedArray("controls");
  JsonObject o = c.createNestedObject();
  o["id"] = "power"; o["type"] = "switch"; o["label"] = "Power"; o["value"] = true;

  o = c.createNestedObject();
  o["id"] = "speed"; o["type"] = "slider"; o["label"] = "Speed";
  o["min"] = 1; o["max"] = 20; o["value"] = 6;

  o = c.createNestedObject();
  o["id"] = "bright"; o["type"] = "slider"; o["label"] = "Brightness";
  o["min"] = 0; o["max"] = 255; o["value"] = BRIGHTNESS;

  o = c.createNestedObject();
  o["id"] = "color"; o["type"] = "color"; o["label"] = "Color"; o["value"] = "#FF0000";

  String out;
  serializeJson(doc, out);
  Serial.println(out);
}

int gSpeed = 6;
CRGB gFixed = CRGB::Red;
bool gUseFixed = false;

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
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, line)) continue;
    if (doc["cmd"] == "describe") sendDescribe();
    else if (doc["cmd"] == "set") applySet(doc.as<JsonObject>());
    else if (doc["cmd"] == "press") { /* кнопок нет */ }
  }

  uint32_t now = millis();
  if (gPower && now - gLast > 30) {
    gLast = now;
    if (gUseFixed) {
      // статичный выбранный цвет с лёгким "дыханием"
      float b = 0.5f + 0.5f * sin(gPhase);
      leds[0] = gFixed;
      leds[0].nscale8(120 + 135 * b);
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