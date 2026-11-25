#include <ArduinoJson.h>

const int redPin = 2;   // Пин для красного цвета
const int greenPin = 4; // Пин для зеленого цвета
const int bluePin = 5;  // Пин для синего цвета

int redValue = 0;
int greenValue = 0;
int blueValue = 0;

void handleCommand(const String& command);
void describe();
void setRGB(int red, int green, int blue);

void setup() {
    Serial.begin(115200);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    
    setRGB(redValue, greenValue, blueValue); // Установим начальные значения
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
        } else if (strcmp(cmd, "set") == 0 && doc["module"] == "rgb") {
            const char* id = doc["id"];
            int value = doc["value"];
            if (strcmp(id, "red") == 0) {
                redValue = value;
            } else if (strcmp(id, "green") == 0) {
                greenValue = value;
            } else if (strcmp(id, "blue") == 0) {
                blueValue = value;
            }
            setRGB(redValue, greenValue, blueValue);
        }
    }
}

void setRGB(int red, int green, int blue) {
    analogWrite(redPin, red);
    analogWrite(greenPin, green);
    analogWrite(bluePin, blue);

    // Отправляем обновленное состояние цвета
    String rgbResponse = "{\"type\":\"update\",\"module\":\"rgb\",\"id\":\"color\",\"value\":\"rgb(" + String(red) + ", " + String(green) + ", " + String(blue) + ")\"}";
    Serial.println(rgbResponse);
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"RGB Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"rgb\",\"name\":\"RGB LED\",\"controls\":[{\"id\":\"red\",\"type\":\"slider\",\"label\":\"Red\",\"min\":0,\"max\":255,\"value\":" + String(redValue) + "},{\"id\":\"green\",\"type\":\"slider\",\"label\":\"Green\",\"min\":0,\"max\":255,\"value\":" + String(greenValue) + "},{\"id\":\"blue\",\"type\":\"slider\",\"label\":\"Blue\",\"min\":0,\"max\":255,\"value\":" + String(blueValue) + "}]}]}";
    Serial.println(response);
}