#include <ArduinoJson.h>

const int ledPin = 2; // Пин светодиода
const int buttonPin = 0; // Пин кнопки на плате
bool ledState = false; // Состояние LED

void handleCommand(const String& command);
void describe();
void updateLEDState();

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP); // Используем подтяжку к VCC
    updateLEDState(); // Установка начального состояния
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        handleCommand(command);
    }

    // Проверка состояния кнопки
    if (digitalRead(buttonPin) == LOW) {
        ledState = !ledState; // Переключаем состояние LED
        updateLEDState();
        delay(300); // Задержка для устранения дребезга
    }

    // Отправляем состояние LED каждую секунду
    String response = "{\"type\":\"update\",\"module\":\"led\",\"id\":\"power\",\"value\":" + String(ledState ? "true" : "false") + "}";
    Serial.println(response);
    delay(1000);
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
                ledState = value; // Установите состояние LED по команде
                updateLEDState();
            }
        } else if (strcmp(cmd, "press") == 0 && doc["module"] == "led") {
            const char* id = doc["id"];
            if (strcmp(id, "go") == 0) {
                Serial.println("Button on the app pressed!");
            }
        }
    }
}

void updateLEDState() {
    digitalWrite(ledPin, ledState ? HIGH : LOW);
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"LED Controller\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":" + String(ledState ? "true" : "false") + "},{\"id\":\"go\",\"type\":\"button\",\"label\":\"Press Me\"}]}]}";
    Serial.println(response);
}