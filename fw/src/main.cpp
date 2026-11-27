#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char* AP_SSID = "ESP32-AP";
const char* AP_PASS = "12345678";

bool  gApActive = false;
int   gApChannel = 6;
int   gMaxClients = 4;

void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "WiFi AP Control";
  doc["version"] = "1.0";

  JsonArray mods = doc["modules"].to<JsonArray>();

  JsonObject m = mods.add<JsonObject>();
  m["id"] = "ap";
  m["name"] = "Access Point";

  JsonArray ctrls = m["controls"].to<JsonArray>();

  JsonObject c1 = ctrls.add<JsonObject>();
  c1["id"] = "power";
  c1["type"] = "switch";
  c1["label"] = "AP Enable";
  c1["value"] = gApActive;

  JsonObject c2 = ctrls.add<JsonObject>();
  c2["id"] = "channel";
  c2["type"] = "slider";
  c2["label"] = "Channel";
  c2["min"] = 1;
  c2["max"] = 13;
  c2["value"] = gApChannel;

  JsonObject c3 = ctrls.add<JsonObject>();
  c3["id"] = "maxclients";
  c3["type"] = "slider";
  c3["label"] = "Max Clients";
  c3["min"] = 1;
  c3["max"] = 8;
  c3["value"] = gMaxClients;

  JsonObject c4 = ctrls.add<JsonObject>();
  c4["id"] = "reset";
  c4["type"] = "button";
  c4["label"] = "Restart AP";

  serializeJson(doc, Serial);
  Serial.println();
}

void sendUpdate(const char* module, const char* id, const char* value) {
  JsonDocument doc;
  doc["type"] = "update";
  doc["module"] = module;
  doc["id"] = id;
  doc["value"] = value;
  serializeJson(doc, Serial);
  Serial.println();
}

void startAP() {
  WiFi.softAP(AP_SSID, AP_PASS, gApChannel, false, gMaxClients);
  gApActive = true;
  IPAddress ip = WiFi.softAPIP();
  sendUpdate("ap", "power", "true");
  Serial.printf("AP started: %s, IP: %s, Ch: %d, Max: %d\n", AP_SSID, ip.toString().c_str(), gApChannel, gMaxClients);
}

void stopAP() {
  WiFi.softAPdisconnect(true);
  gApActive = false;
  sendUpdate("ap", "power", "false");
  Serial.println("AP stopped");
}

void handleCmd(JsonDocument& doc) {
  const char* cmd = doc["cmd"];
  const char* module = doc["module"];
  const char* id = doc["id"];

  if (strcmp(cmd, "describe") == 0) {
    sendDescribe();
    return;
  }

  if (strcmp(module, "ap") != 0) return;

  if (strcmp(cmd, "set") == 0) {
    if (strcmp(id, "power") == 0) {
      bool val = doc["value"];
      if (val && !gApActive) startAP();
      else if (!val && gApActive) stopAP();
    }
    else if (strcmp(id, "channel") == 0) {
      gApChannel = doc["value"];
      sendUpdate("ap", "channel", String(gApChannel).c_str());
      if (gApActive) { stopAP(); startAP(); }
    }
    else if (strcmp(id, "maxclients") == 0) {
      gMaxClients = doc["value"];
      sendUpdate("ap", "maxclients", String(gMaxClients).c_str());
      if (gApActive) { stopAP(); startAP(); }
    }
  }
  else if (strcmp(cmd, "press") == 0) {
    if (strcmp(id, "reset") == 0) {
      if (gApActive) { stopAP(); startAP(); }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nWiFi AP Control ready");
  sendDescribe();
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("{")) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, line);
      if (!err) handleCmd(doc);
    }
  }
}