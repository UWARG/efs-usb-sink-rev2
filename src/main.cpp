#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <AP33772S.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 4
#define I2C_SCL 5
#define PWR_EN_PIN 6

String apSSID = "EFS_USBC_Sink_";
const char* AP_PASS = "warg_efs";

AsyncWebServer server(80);
AP33772S usbpd; 
Adafruit_INA219 ina219;

float currentTargetVoltage = 5.0; 
bool systemEnabled = false;

String updatePDController(float voltage, float current, bool enable) {
    if (!enable) {
        digitalWrite(PWR_EN_PIN, LOW);
        usbpd.setFixPDO(1, 1000); 
        systemEnabled = false;
        Serial.println("SYS_OFF: Disabled Output (PWR_EN = LOW).");
        return ""; 
    }

    int current_ma = (int)(current * 1000.0);
    String errorReason = "";
    float expectedV = voltage;

    if (voltage < 4.0) {
        usbpd.setFixPDO(1, current_ma); 
        expectedV = 5.0; // AP33772S must provide 5V to the LDO
    } else if (voltage > 15.5 && voltage < 16.5) {
        int ppsIndex = usbpd.getPPSIndex(); 
        if (ppsIndex > 0) {
            usbpd.setPPSPDO(ppsIndex, 16000, current_ma);
        } else {
            // No fallback allowed: explicitly fail if PPS is unavailable
            errorReason = "PPS unavailable on source (Cannot reach 16V).";
            usbpd.setFixPDO(1, current_ma);
            expectedV = 5.0;
        }
    } else {
        int pdoIndex = 1; 
        if (voltage >= 20.0) pdoIndex = 5;
        else if (voltage >= 15.0) pdoIndex = 4;
        else if (voltage >= 12.0) pdoIndex = 3;
        else if (voltage >= 5.0) pdoIndex = 1;
        
        usbpd.setFixPDO(pdoIndex, current_ma);
    }
    
    // Give the PD IC time to negotiate before reading the actual bus voltage
    delay(1000); 

    if (errorReason == "") {
        float actualV = (float)usbpd.readVoltage() / 1000.0;
        
        // 1.5V tolerance accounts for heavy loads and voltage sag
        if (actualV < expectedV - 1.5) { 
            errorReason = "Source rejected request (Outputting " + String(actualV, 1) + "V max).";
        }
    }

    // Safety fallback: if negotiation failed, abort enabling the output
    if (errorReason != "") {
        digitalWrite(PWR_EN_PIN, LOW); 
        usbpd.setFixPDO(1, 1000); // Revert to 5V
        systemEnabled = false;
        Serial.println("FAULT: " + errorReason);
        return errorReason;
    }

    currentTargetVoltage = voltage;
    systemEnabled = true;
    digitalWrite(PWR_EN_PIN, HIGH);
    
    Serial.printf("SYS_ON: Achieved target voltage, Output enabled.\n");
    return "";
}

void setup() {
    Serial.begin(115200);
    
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, LOW); 
    
    Wire.begin(I2C_SDA, I2C_SCL);
    
    usbpd.begin(); 
    usbpd.setFixPDO(1, 1000); 

    if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
    }

    if(!LittleFS.begin(true)){
        Serial.println("LittleFS Mount Failed");
        return;
    }

    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    char macHex[9];
    snprintf(macHex, sizeof(macHex), "%02X%02X%02X%02X", baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    apSSID += String(macHex);

    // Initialize WiFi AP with the unique name
    WiFi.softAP(apSSID.c_str(), AP_PASS);
    Serial.print("AP SSID: ");
    Serial.println(apSSID);

    server.on("/api/set", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data);
            
            if (error) {
                request->send(400, "text/plain", "Bad Request");
                return;
            }

            float v = doc["voltage"];
            float i = doc["current"];
            bool en = doc["enable"];

            String result = updatePDController(v, i, en);
            
            if (result == "") {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                String jsonErr = "{\"status\":\"error\",\"message\":\"" + result + "\"}";
                request->send(400, "application/json", jsonErr);
            }
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        float busvoltage = 0.0;
        float current_mA = 0.0;

        if (systemEnabled) {
            if (currentTargetVoltage < 4.0) {
                busvoltage = ina219.getBusVoltage_V();
                current_mA = ina219.getCurrent_mA() * 10.0; // Note: Multiply by 10 as we use a 10mΩ shunt resistor instead of standard 0.1Ω
                current_mA *= -1.0; // IN+/IN- are reversed in our circuit, so current needs to be negated
            } else {
                busvoltage = (float)usbpd.readVoltage() / 1000.0; 
                current_mA = (float)usbpd.readCurrent();          
            }
        }
        
        String json = "{\"voltage\":";
        json += String(busvoltage, 2);
        json += ",\"current\":";
        json += String(current_mA / 1000.0, 3);
        json += "}";
        
        request->send(200, "application/json", json);
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(204);
    });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.begin();
    Serial.println("System Initialized.");
}

void loop() {
    // Managed asynchronously
}
