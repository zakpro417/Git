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

static void pushUpdateStr(const char *id, const String &val) {
  JsonDocument d;
  d["type"] = "update";
  d["module"] = "wifi";
  d["id"] = id;
  d["value"] = val;
  sendJson(d);
}

static void pushUpdateBool(const char *id, bool val) {
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
    pushUpdateBool("ap", true);
    pushUpdateStr("ssid", AP_SSID);
    pushUpdateStr("ip", WiFi.softAPIP().toString());
    pushUpdateStr("status", "AP active");
  } else {
    WiFi.softAPdisconnect(true);
    pushUpdateBool("ap", false);
    pushUpdateStr("ip", "-");
    pushUpdateStr("status", "Idle");
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
  JsonObject power = c.add<JsonObject>();
  power["id"] = "power";
  power["type"] = "switch";
  power["label"] = "AP Enable";
  power["value"] = apOn;
  JsonObject ch = c.add<JsonObject>();
  ch["id"] = "channel";
  ch["type"] = "slider";
  ch["label"] = "Channel";
  ch["min"] = 1;
  ch["max"] = 13;
  ch["value"] = 6;
  JsonObject mc = c.add<JsonObject>();
  mc["id"] = "maxclients";
  mc["type"] = "slider";
  mc["label"] = "Max Clients";
  mc["min"] = 1;
  mc["max"] = 8;
  mc["value"] = 4;
  JsonObject status = c.add<JsonObject>();
  status["id"] = "status";
  status["type"] = "readout";
  status["label"] = "Status";
  status["value"] = apOn ? "AP active" : "Idle";
  sendJson(doc);
}

static void handleLine(const String &line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) return;
  const char *cmd = doc["cmd"] | "";
  if (!strcmp(cmd, "describe")) { sendDescribe(); return; }
  if (!strcmp(cmd, "set") && !strcmp(doc["id"] | "", "power")) {
    setAp(doc["value"].as<bool>());
  }
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