#include "pins.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include "Cipher.h"
#include "TFT_eSPI.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include "SHT85.h"  // Correct header file for SHT30
#include "image/logo.h"

TFT_eSPI tft = TFT_eSPI();
#define WAIT 1000

WiFiClient espClient;
PubSubClient client(espClient);
char boardID[23];
const char* versionUrl = "http://10.147.86.130:5000/version";
#include "src\function.h"

#define SHT30_ADDRESS  0x44
float temperature = 0;
float humidity =0;
char temp_humid;
SHT30 sht(SHT30_ADDRESS);

char key[32] = "abcdefghijklmnop";
Cipher *cipher = new Cipher();
int a = 0;
int counter = 0;
long lastMsg = 0;
char msg[50];
int value = 0;
float temperature_list[50];
float humidity_list[50];

float avr_temperature = 0;
float avr_humidity = 0;

uint32_t start;
uint32_t stop;
// Function declarations

void drawRGBImage(int x, int y, int w, int h, const uint16_t *image) {
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  for (int i = 0; i < w * h; i++) {
    tft.pushColor(pgm_read_word(&image[i]));
  }
  tft.endWrite();
}

void setup() {
    ////////////////
    for (int i = 49; i > 0; i--) {
        temperature_list[i] = 0;
        humidity_list[i] = 0;
    }
    ////////////////

    Serial.begin(115200);
    delay(1000);
    snprintf(boardID, 23, "%llX", ESP.getEfuseMac());
    
    //////////////////////////////
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_WHITE);
    drawRGBImage(0, 0, 240, 320, gImage_logo);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    /////////////////////////////

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    ////////////////////
    
    cipher->setKey(key);

    // Initialize SD card
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);
    bool rlst = SD_MMC.begin("/sdcard", true);
    if (!rlst) {
        Serial.println("SD init failed");
        Serial.println("➸ No detected SdCard");
        while (1) delay(1); // Stop here if SD card init fails
    } else {
        Serial.println("SD init success");
        Serial.printf("➸ Detected SdCard insert: %.2f GB\r\n", SD_MMC.cardSize() / 1024.0 / 1024.0 / 1024.0);
    }
    
    // Read data from the file
    File file = SD_MMC.open("/Jsondata.json");
    if (!file) {
        Serial.println("Failed to open file for reading");
        while (1) delay(1); // Stop here if JSON file read fails
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
        DynamicJsonDocument doc(1024); // Adjust the size as per your JSON size
        DeserializationError error = deserializeJson(doc, fileContent);
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.f_str());
            while (1) delay(1); // Stop here if JSON parsing fails
        }

        // Check and encode/decode WiFi and MQTT passwords
        const char* wifi_pwd = doc["wifi_pwd"];
        const char* wifi_encoded = doc["wifi_encoded"];
        const char* mqtt_pwd = doc["mqtt_pwd"];
        const char* mqtt_encoded = doc["mqtt_encoded"];
        
        if (strcmp(wifi_encoded, "False") == 0 || strcmp(mqtt_encoded, "False") == 0) {
            if (strcmp(wifi_encoded, "False") == 0) {
                String wifi_pwd_encrypted = cipher->encryptString(wifi_pwd);
                doc["wifi_pwd"] = wifi_pwd_encrypted; 
                doc["wifi_encoded"] = "True";
            }

            if (strcmp(mqtt_encoded, "False") == 0) {
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

        // Decrypt WiFi and MQTT passwords
        String wifi_pwd_decrypted = cipher->decryptString(doc["wifi_pwd"]);
        String mqtt_pwd_decrypted = cipher->decryptString(doc["mqtt_pwd"]);

        // Copy configurations
        strlcpy(ssid, doc["wifi_ssid"], sizeof(ssid));
        strlcpy(password, wifi_pwd_decrypted.c_str(), sizeof(password));
        strlcpy(mqtt_server, doc["mqtt_broker"], sizeof(mqtt_server));
        strlcpy(mqtt_username, doc["mqtt_user"], sizeof(mqtt_username));
        strlcpy(device_name, doc["DeviceName"] | "TEMP0022", sizeof(device_name)); // Ensure device_name is correctly assigned
        strlcpy(mqtt_password, mqtt_pwd_decrypted.c_str(), sizeof(mqtt_password));
    
    
        setup_wifi();
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

        ///////////////////////////////////////////////////////////////////////////////
        // Check for OTA updates
        if (checkForUpdates()) {
            downloadAndUpdate();
        } else {
            Serial.println("No new version available.");
        }

        // Initialize SHT30 sensor
        Wire.setPins(16, 15); // Set I2C pins: SDA to pin 16, SCL to pin 15
        Wire.begin();
        Wire.setClock(100000);
        if (!sht.begin()) {
            Serial.println("Couldn't find SHT30");
            while (1) delay(1); // Stop here if SHT30 initialization fails
        }
        uint16_t stat = sht.readStatus();
        Serial.print(stat, HEX);
        Serial.print("Board ID: ");
        Serial.print(boardID);
        Serial.println();
    }
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    // Add code here for other tasks to run in loop
    sht.read();
    temperature = sht.getTemperature();
    humidity = sht.getHumidity();
    Serial.print(temperature);
    Serial.print(humidity);
    for (int i = 49; i > 0; i--){

        temperature_list[i] = temperature_list[i - 1];
        temperature_list[0] = temperature;
        humidity_list[i] = humidity_list[i - 1];
        humidity_list[0] = humidity;

        for (int j = 0; j < 50; j++){
            avr_temperature += temperature_list[j];
            avr_humidity    += humidity_list[j];
        }
        avr_temperature /= 51;
        avr_humidity    /= 51;
    }
    
    long now = millis();
    if (now - lastMsg > 30000) {
        lastMsg = now;
        counter ++;
        // Read temperature and humidity
        start = micros();
        stop = micros();

        tft.setCursor(84,105);
        tft.setTextSize(3);
        tft.print(avr_temperature);

        tft.setCursor(84,208);
        tft.setTextSize(3);
        tft.print(avr_humidity);

        if (!isnan(avr_temperature) && !isnan(avr_humidity)) {
        // Format and publish temperature and humidity
        JsonDocument doc;
        doc["boardID"]     = boardID;
        doc["temperature"] = avr_temperature;
        doc["humidity"]    = avr_humidity;
        doc["displayName"] = device_name;
        doc["org"]  = "TG3";
        doc["dept"] = "Test Lab";
        doc["room"] = "ATF";
        doc["line"] = "123";
        doc["low_temp"] = 20;
        doc["high_temp"] = 28;
        doc["low_humid"] = 40;
        doc["high_humid"] = 80;
        // Serialize JSON to string
        char jsonBuffer[512];
        serializeJson(doc, jsonBuffer);

        // Publish JSON data over MQTT
        client.publish("VNTESTLAB/TEMP&HUMID", jsonBuffer);
        } else {
            Serial.println("Failed to read from SHT30 sensor!");
        }       
    }      
}


    
    


