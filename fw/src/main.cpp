#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static const char *AP_SSID = "ESP-Control";
static const char *AP_PASS = "12345678";
static bool apOn = false;

static void sendJson(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

static void pushUpdate(const char *id, const JsonVariantConst &val) {
  JsonDocument d;
  d["type"] = "update";
  d["module"] = "wifi";
  d["id"] = id;
  d["value"] = val;
  sendJson(d);
}

static void setAp(bool on) {
  apOn = on;
  if (on) {
    WiFi.softAP(AP_SSID, AP_PASS);
    pushUpdate("ap", true);
    pushUpdate("ssid", AP_SSID);
    pushUpdate("ip", WiFi.softAPIP().toString().c_str());
    pushUpdate("status", "AP active");
  } else {
    WiFi.softAPdisconnect(true);
    pushUpdate("ap", false);
    pushUpdate("ip", "-");
    pushUpdate("status", "Idle");
  }
}

static void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "WiFi AP Minimal";
  doc["version"] = "1.0";
  JsonArray mods = doc["modules"].to<JsonArray>();
  JsonObject m = mods.add<JsonObject>();
  m["id"] = "wifi";
  m["name"] = "Access Point";
  JsonArray c = m["controls"].to<JsonArray>();

  JsonObject ap = c.add<JsonObject>();
  ap["id"] = "ap";
  ap["type"] = "switch";
  ap["label"] = "AP Enable";
  ap["value"] = apOn;

  JsonObject ssid = c.add<JsonObject>();
  ssid["id"] = "ssid";
  ssid["type"] = "readout";
  ssid["label"] = "SSID";
  ssid["value"] = AP_SSID;

  JsonObject ip = c.add<JsonObject>();
  ip["id"] = "ip";
  ip["type"] = "readout";
  ip["label"] = "IP";
  ip["value"] = "-";

  JsonObject status = c.add<JsonObject>();
  status["id"] = "status";
  status["type"] = "readout";
  status["label"] = "Status";
  status["value"] = "Idle";

  sendJson(doc);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting...");
  sendDescribe();
  Serial.println("Setup done");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("{")) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) return;

    const char *cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "describe") == 0) {
      sendDescribe();
    } else if (strcmp(cmd, "set") == 0) {
      const char *mod = doc["module"];
      const char *id = doc["id"];
      if (mod && id && strcmp(mod, "wifi") == 0 && strcmp(id, "ap") == 0) {
        setAp(doc["value"].as<bool>());
      }
    }
  }
}