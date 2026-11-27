#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char* AP_SSID = "ESP32-AP";
const char* AP_PASS = "12345678";

bool gApActive = false;
int  gApChannel = 6;
int  gMaxClients = 4;
unsigned long gLastStatus = 0;

static void sendDescribe() {
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
  power["value"] = gApActive;

  JsonObject ch = c.add<JsonObject>();
  ch["id"] = "channel";
  ch["type"] = "slider";
  ch["label"] = "Channel";
  ch["min"] = 1;
  ch["max"] = 13;
  ch["value"] = gApChannel;

  JsonObject mc = c.add<JsonObject>();
  mc["id"] = "maxclients";
  mc["type"] = "slider";
  mc["label"] = "Max Clients";
  mc["min"] = 1;
  mc["max"] = 8;
  mc["value"] = gMaxClients;

  JsonObject st = c.add<JsonObject>();
  st["id"] = "status";
  st["type"] = "readout";
  st["label"] = "Status";
  st["value"] = gApActive ? (String("AP ") + AP_SSID + " ip " + WiFi.softAPIP().toString() + " clients " + String(WiFi.softAPgetStationNum())) : String("AP off");

  JsonObject btn = c.add<JsonObject>();
  btn["id"] = "restart";
  btn["type"] = "button";
  btn["label"] = "Restart AP";

  serializeJson(doc, Serial);
  Serial.println();
}

static void pushStatus() {
  JsonDocument doc;
  doc["type"] = "update";
  doc["module"] = "ap";
  doc["id"] = "status";
  doc["value"] = gApActive ? (String("AP ") + AP_SSID + " ip " + WiFi.softAPIP().toString() + " clients " + String(WiFi.softAPgetStationNum())) : String("AP off");
  serializeJson(doc, Serial);
  Serial.println();
}

static void startAP() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS, gApChannel, 0, gMaxClients);
  Serial.println(ok ? String("AP started ch ") + String(gApChannel) : "AP start FAILED");
  gApActive = ok;
}

static void stopAP() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  gApActive = false;
  Serial.println("AP stopped");
}

static void restartAP() {
  stopAP();
  delay(200);
  startAP();
  pushStatus();
}

static void handleSet(const char* id, JsonVariant value) {
  if (strcmp(id, "power") == 0) {
    bool on = value.as<bool>();
    if (on == gApActive) return;
    if (on) startAP(); else stopAP();
    pushStatus();
  } else if (strcmp(id, "channel") == 0) {
    int ch = constrain((int)value.as<int>(), 1, 13);
    if (ch == gApChannel) return;
    gApChannel = ch;
    if (gApActive) restartAP();
    Serial.println(String("channel ") + String(gApChannel));
  } else if (strcmp(id, "maxclients") == 0) {
    int mc = constrain((int)value.as<int>(), 1, 8);
    if (mc == gMaxClients) return;
    gMaxClients = mc;
    if (gApActive) restartAP();
    Serial.println(String("maxclients ") + String(gMaxClients));
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi AP Control ready");
  sendDescribe();
}

void loop() {
  static String buf;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n') {
      buf.trim();
      if (buf.startsWith("{")) {
        JsonDocument doc;
        if (!deserializeJson(doc, buf)) {
          const char* cmd = doc["cmd"] | "";
          if (strcmp(cmd, "describe") == 0) {
            sendDescribe();
          } else if (strcmp(cmd, "set") == 0) {
            handleSet(doc["id"] | "", doc["value"]);
          } else if (strcmp(cmd, "press") == 0) {
            const char* id = doc["id"] | "";
            if (strcmp(id, "restart") == 0) restartAP();
          }
        } else {
          Serial.println("JSON error");
        }
      }
      buf = "";
    } else if (buf.length() < 400) {
      buf += ch;
    }
  }

  if (gApActive && millis() - gLastStatus > 2000) {
    gLastStatus = millis();
    pushStatus();
  }
}