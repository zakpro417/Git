#include <Arduino.h>
#include <ArduinoJson.h>

#define SERIAL_BAUD 115200

void setup() {
    Serial.begin(SERIAL_BAUD);
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        DynamicJsonDocument doc(256);
        deserializeJson(doc, command);

        if (doc["cmd"] == "describe") {
            String response = "{\"type\":\"describe\",\"firmware\":\"LED Controller\",\"version\":\"1.0\",\"modules\":[{\"id\":\"led\",\"name\":\"LED Strip\",\"controls\":[{\"id\":\"power\",\"type\":\"switch\",\"label\":\"Power\",\"value\":true},{\"id\":\"brightness\",\"type\":\"slider\",\"label\":\"Brightness\",\"min\":0,\"max\":255,\"value\":128},{\"id\":\"color\",\"type\":\"color\",\"label\":\"Color\",\"value\":\"#0000FF\"},{\"id\":\"go\",\"type\":\"button\",\"label\":\"Run\"},{\"id\":\"temp\",\"type\":\"readout\",\"label\":\"Temperature\",\"value\":\"21.5 C\"}]}]}";
            Serial.println(response);
        }
    }
}