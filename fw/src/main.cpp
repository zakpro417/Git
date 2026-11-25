#include <Arduino.h>
#include <ArduinoJson.h>

#define LED_PIN 2
bool power = true;
int brightness = 128;
String color = "#0000FF";

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    updateLED();
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        handleCommand(line);
    }
}

void handleCommand(String cmd) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, cmd);
    const char* command = doc["cmd"];
    
    if (strcmp(command, "describe") == 0) {
        describe();
    } else if (strcmp(command, "set") == 0) {
        const char* module = doc["module"];
        const char* id = doc["id"];
        if (strcmp(module, "led") == 0) {
            if (strcmp(id, "power") == 0) {
                power = doc["value"];
                updateLED();
            } else if (strcmp(id, "brightness") == 0) {
                brightness = doc["value"];
                updateLED();
            } else if (strcmp(id, "color") == 0) {
                color = doc["value"];
                updateLED();
            }
        }
    } else if (strcmp(command, "press") == 0) {
        // Handle button press events here
    }
}

void describe() {
    String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"LED Strip\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":";
    response += power ? "true" : "false";
    response += "},{\"id\":\"brightness\",\"type\":\"slider\",\"label\":\"Brightness\",\"min\":0,\"max\":255,\"value\":";
    response += brightness;
    response += "},{\"id\":\"color\",\"type\":\"color\",\"label\":\"Color\",\"value\":\"";
    response += color;
    response += "\"},{\"id\":\"go\",\"type\":\"button\",\"label\":\"Run\"},{\"id\":\"temp\",\"type\":\"readout\",\"label\":\"Temperature\",\"value\":\"21.5 C\"}]}]}";
    Serial.println(response);
}

void updateLED() {
    if (power) {
        Serial.println("LED is ON");
    } else {
        Serial.println("LED is OFF");
    }
}