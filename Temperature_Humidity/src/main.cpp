#include "pins.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include "Cipher.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include "SHT85.h"  // Correct header file for SHT30

#define SHT30_ADDRESS  0x44
char boardID[23];

WiFiClient espClient;
PubSubClient client(espClient);
SHT30 sht(SHT30_ADDRESS);

char* key = "abcdefghijklmnop";
Cipher *cipher = new Cipher();

long lastMsg = 0;
char msg[50];
int value = 0;

char ssid[32];
char password[64];
char mqtt_password[64];

const char* mqtt_server;
const char* mqtt_username;
char device_name[64];  // Allocate sufficient space for device_name

uint32_t start;
uint32_t stop;

// Function declarations
void setup_wifi();
void callback(char* topic, byte* message, unsigned int length);
void reconnect();

void setup() {
    Serial.begin(115200);
    delay(1000);
    snprintf(boardID, 23, "%llX", ESP.getEfuseMac());
    
    cipher->setKey(key);

    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);
    bool rlst = SD_MMC.begin("/sdcard", true);
    if (!rlst) {
        Serial.println("SD init failed");
        Serial.println("➸ No detected SdCard");
        return;
    } else {
        Serial.println("SD init success");
        Serial.printf("➸ Detected SdCard insert: %.2f GB\r\n", SD_MMC.cardSize() / 1024.0 / 1024.0 / 1024.0);
    }
   
    // Read data from the file
    File file = SD_MMC.open("/Jsondata.json");
    if (!file) {
        Serial.println("Failed to open file for reading");
    } else {
        Serial.println("Reading from file:");
        String fileContent;
        while (file.available()) {
            fileContent += (char)file.read();
        }
        file.close();
        Serial.println(fileContent);
        Serial.println("\nRead complete....");

        // Parse JSON data
        DynamicJsonDocument doc(4096);  // Increased size to handle larger JSON content
        DeserializationError error = deserializeJson(doc, fileContent);
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.f_str());
            return;
        }
        // Extract and print JSON data
        String wifi_pwd = doc["wifi_pwd"];
        String wifi_encoded = doc["wifi_encoded"];
        String mqtt_pwd = doc["mqtt_pwd"];
        String mqtt_encoded = doc["mqtt_encoded"];

        if (wifi_encoded == "False" || mqtt_encoded == "False") {

            if (wifi_encoded == "False") {
                String wifi_pwd_encrypted = cipher->encryptString(wifi_pwd);
                doc["wifi_pwd"] = wifi_pwd_encrypted; 
                doc["wifi_encoded"] = "True";
            }

            if (mqtt_encoded == "False") {
                String mqtt_pwd_encrypted = cipher->encryptString(mqtt_pwd);
                doc["mqtt_pwd"] = mqtt_pwd_encrypted;
                doc["mqtt_encoded"] = "True";   
            }
            
            // Serialize JSON data back to a string
            String newJsonData;
            serializeJson(doc, newJsonData);

            // Write modified JSON data back to the file
            file = SD_MMC.open("/Jsondata.json", FILE_WRITE);
            if (!file) {
                Serial.println("Failed to open file for writing");
            } else {  
                file.println(newJsonData);
                file.close();
                Serial.println("Write complete.");
            }
        }

        String wifi_pwd_decrypted = cipher->decryptString(doc["wifi_pwd"]);
        String mqtt_pwd_decrypted = cipher->decryptString(doc["mqtt_pwd"]);

        strlcpy(ssid, doc["wifi_ssid"], sizeof(ssid));
        strlcpy(password, wifi_pwd_decrypted.c_str(), sizeof(password));
        
        mqtt_server = doc["mqtt_broker"];

        mqtt_username = doc["mqtt_user"];
        strlcpy(device_name, doc["DeviceName"] | "TEMP0022", sizeof(device_name));  // Ensure device_name is correctly assigned
        strlcpy(mqtt_password, mqtt_pwd_decrypted.c_str(), sizeof(mqtt_password));

        setup_wifi();
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

        // Initialize SHT30 sensor
        Wire.setPins(16, 15); // Set I2C pins: SDA to pin 16, SCL to pin 15
        Wire.begin();
        Wire.setClock(100000);
        if (!sht.begin()) {
            Serial.println("Couldn't find SHT30");
            while (1) delay(1);
        }
        uint16_t stat = sht.readStatus();
        Serial.print(stat, HEX);
        Serial.println();
    }
}

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

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
            client.subscribe("HoangAnh/test");
            // Publish board ID once after connecting
            client.publish("HoangAnh/test/boardID", boardID);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    // Add code here for other tasks to run in loop

    long now = millis();
    if (now - lastMsg > 2000) {
        lastMsg = now;
        // Read temperature and humidity
        start = micros();
        sht.read();
        stop = micros();

        float temperature = sht.getTemperature();
        float humidity = sht.getHumidity();

        if (!isnan(temperature) && !isnan(humidity)) {
            // Format and publish temperature and humidity
            DynamicJsonDocument doc(512);
            doc["boardID"] = boardID;
            doc["temperature"] = temperature;
            doc["humidity"] = humidity;
            doc["DeviceName"] = device_name;

            // Serialize JSON to string
            char jsonBuffer[512];
            serializeJson(doc, jsonBuffer);

            // Publish JSON data over MQTT
            client.publish("HoangAnh/test/data", jsonBuffer);
        } else {
            Serial.println("Failed to read from SHT30 sensor!");
        }

        // Print the time taken for the read operation
        Serial.print("\tTime taken: ");
        Serial.print((stop - start) * 0.001);
        Serial.println(" ms");
    }
}
