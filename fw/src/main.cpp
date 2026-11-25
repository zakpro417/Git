#include <ArduinoJson.h>

const int ledPin = 2; // Пин встроенного LED на плате
bool ledState = false; // Состояние LED

void handleCommand(const String& command);
void describe();

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // Выключаем LED изначально
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        handleCommand(command);
    }
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
                ledState = value;  // Устанавливаем состояние LED по команде
                digitalWrite(ledPin, ledState ? HIGH : LOW);
            }
        }
    }
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"Built-in LED\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":" + String(ledState ? "true" : "false") + "}]}]}";
    Serial.println(response);
}