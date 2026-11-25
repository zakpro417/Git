#include <ArduinoJson.h>

const int ledPin = 2; // Пин светодиода

void handleCommand(const String& command);
void describe();

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        handleCommand(command);
    }

    // Предположим, здесь может быть код для обновления состояния, если требуется.
}

void handleCommand(const String& command) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, command);

    if (doc["cmd"].is<String>()) {
        const char* cmd = doc["cmd"];

        if (strcmp(cmd, "describe") == 0) {
            describe();
        } else if (strcmp(cmd, "set") == 0 && doc["module"] == "led") {
            const char* id = doc["id"];
            bool value = doc["value"];
            if (strcmp(id, "power") == 0) {
                digitalWrite(ledPin, value ? HIGH : LOW);
            }
        } else if (strcmp(cmd, "press") == 0 && doc["module"] == "led") {
            const char* id = doc["id"];
            if (strcmp(id, "go") == 0) {
                Serial.println("Button pressed!");
            }
        }
    }
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"LED Controller\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":false},{\"id\":\"go\",\"type\":\"button\",\"label\":\"Press Me\"}]}]}";
    Serial.println(response);
}