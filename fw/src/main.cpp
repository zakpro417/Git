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

static void pushUpdate(const char *id, const String &val) {
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
    pushUpdate("ip", WiFi.softAPIP().toString());
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
  JsonObject sw = c.add<JsonObject>();
  sw["id"] = "ap";
  sw["type"] = "switch";
  sw["label"] = "Access Point";
  sw["value"] = apOn;
  JsonObject s = c.add<JsonObject>();
  s["id"] = "ssid";
  s["type"] = "readout";
  s["label"] = "SSID";
  s["value"] = AP_SSID;
  JsonObject ip = c.add<JsonObject>();
  ip["id"] = "ip";
  ip["type"] = "readout";
  ip["label"] = "IP";
  ip["value"] = apOn ? WiFi.softAPIP().toString() : "-";
  JsonObject st = c.add<JsonObject>();
  st["id"] = "status";
  st["type"] = "readout";
  st["label"] = "Status";
  st["value"] = apOn ? "AP active" : "Idle";
  sendJson(doc);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  sendDescribe();
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("{")) return;
    JsonDocument doc;
    if (deserializeJson(doc, line)) return;
    const char *cmd = doc["cmd"] | "";
    if (strcmp(cmd, "describe") == 0) {
      sendDescribe();
    } else if (strcmp(cmd, "set") == 0) {
      const char *id = doc["id"] | "";
      if (strcmp(id, "ap") == 0) setAp(doc["value"] | false);
    }
  }
  delay(10);
}