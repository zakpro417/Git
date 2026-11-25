#include <ArduinoJson.h>

const int ledPin = 2; // Пин светодиода
float temperature = 20.0; // Начальная температура

void handleCommand(const String& command);
void describe();
void updateTemperature();

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

    // Обновлять температуру каждые 5 секунд
    updateTemperature();
    delay(5000);
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

void updateTemperature() {
    // Имитация изменения температуры
    temperature += (random(-1, 2)); // Случайное изменение температуры
    String response = "{\"type\":\"update\",\"module\":\"led\",\"id\":\"temp\",\"value\":\"" + String(temperature) + " C\"}";
    Serial.println(response);
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"LED Controller\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":false},{\"id\":\"go\",\"type\":\"button\",\"label\":\"Press Me\"},{\"id\":\"temp\",\"type\":\"readout\",\"label\":\"Temperature\",\"value\":\"" + String(temperature) + " C\"}]}]}";
    Serial.println(response);
}