#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

String currentVersion = "";
String latestVersion = "";
String filename = "";

char ssid[32];
char password[64];
char mqtt_password[64];

char mqtt_server[64];
char mqtt_username[64];
char device_name[64];  // Allocate sufficient space for device_name

#define EEPROM_SIZE 64
#define VERSION_ADDR 0

bool checkForUpdates() {
    HTTPClient http;
    http.begin(versionUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Received payload: " + payload);
        JsonDocument doc;
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
    String firmwareUrl = "http://10.147.86.130:5000/download/" + filename;
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



void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin("TTi Factory", "ttiFactory19@1");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();
}

void reconnect() {
    // Loop until we're reconnected
    
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        

        // Attempt to connect with MQTT username and password
        if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
            Serial.println("connected");
            // Subscribe to topic
            client.subscribe("VNTESTLAB/TEMP&HUMID/Control/boardID");
            // Publish board ID once after connecting
            client.publish("VNTESTLAB/TEMP&HUMID/boardID", boardID);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
