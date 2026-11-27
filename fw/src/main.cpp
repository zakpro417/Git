#include <Arduino.h>
#include <ArduinoJson.h>

void sendDescribe() {
  JsonDocument doc;
  doc["type"] = "describe";
  doc["firmware"] = "WiFi AP Control";
  doc["version"] = "1.0";

  JsonArray mods = doc["modules"].to<JsonArray>();
  JsonObject m = mods.add<JsonObject>();
  m["id"] = "ap";
  m["name"] = "Access Point";

  JsonArray c = m["controls"].to<JsonArray>();

  JsonObject power = c.add<JsonObject>();
  power["id"] = "power";
  power["type"] = "switch";
  power["label"] = "AP Enable";
  power["value"] = false;

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

  JsonObject restart = c.add<JsonObject>();
  restart["id"] = "restart";
  restart["type"] = "button";
  restart["label"] = "Restart AP";

  JsonObject status = c.add<JsonObject>();
  status["id"] = "status";
  status["type"] = "readout";
  status["label"] = "Status";
  status["value"] = "Idle";

  serializeJson(doc, Serial);
  Serial.println();
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
    if (line.startsWith("{")) {
      Serial.println("Received: " + line);
    }
  }
  delay(100);
}