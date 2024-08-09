#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

const char* ssid = "Hoàng Anh";
const char* password = "12345678";

const char* versionUrl = "http://192.168.14.145:5000/version";
String currentVersion = "";
String latestVersion = "";
String filename = "";

#define EEPROM_SIZE 64
#define VERSION_ADDR 0

bool checkForUpdates() {
    HTTPClient http;
    http.begin(versionUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Received payload: " + payload);
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        latestVersion = doc["version"].as<String>();
        filename = doc["filename"].as<String>();
        Serial.println("File name: " + filename);
        Serial.println("Latest version: " + latestVersion);
        if (latestVersion != currentVersion) {
            Serial.println("New version available: " + latestVersion);
            return true;
        }
    } else {
        Serial.printf("HTTP GET failed: %d\n", httpCode);
    }
    http.end();
    return false;
}

String readVersionFromEEPROM() {
    char version[EEPROM_SIZE];
    for (int i = 0; i < EEPROM_SIZE; i++) {
        version[i] = EEPROM.read(VERSION_ADDR + i);
        if (version[i] == '\0') break;
    }
    return String(version);
}

void saveVersionToEEPROM(String version) {
    for (int i = 0; i < version.length(); i++) {
        EEPROM.write(VERSION_ADDR + i, version[i]);
    }
    // Write a null terminator
    EEPROM.write(VERSION_ADDR + version.length(), '\0');
    EEPROM.commit();
}

void downloadAndUpdate() {
    HTTPClient http;
    String firmwareUrl = "http://192.168.14.145:5000/download/" + filename;
    http.begin(firmwareUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        WiFiClient* stream = http.getStreamPtr();
        if (contentLength > 0) {
            bool canBegin = Update.begin(contentLength);
            if (canBegin) {
                size_t written = Update.writeStream(*stream);
                if (written == contentLength) {
                    Serial.println("Update successfully written");
                    if (Update.end()) {
                        if (Update.isFinished()) {
                            Serial.println("Update successfully completed");
                            // Save the new version to EEPROM
                            saveVersionToEEPROM(latestVersion);
                            delay(1000);
                            ESP.restart();
                        } else {
                            Serial.println("Update not finished");
                        }
                    } else {
                        Serial.printf("Update error: %s\n", Update.errorString());
                    }
                } else {
                    Serial.println("Written file size does not match content length");
                }
            } else {
                Serial.println("Not enough space to begin OTA");
            }
        } else {
            Serial.println("Empty payload received");
        }
    } else {
        Serial.printf("HTTP GET failed: %d\n", httpCode);
    }
    http.end();
}


void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    // Read the current version from EEPROM
    currentVersion = readVersionFromEEPROM();
    Serial.println("Current version: " + currentVersion);

    if (checkForUpdates()) {
        downloadAndUpdate();
    } else {
        Serial.println("Don't have new version");
    }
}




void loop() {
 Serial.println("1");
}
