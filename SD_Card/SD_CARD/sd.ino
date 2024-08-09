#include "pins.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include "Cipher.h"

/* const char* jsonData = R"(
{"wifi_ssid":"TTIIoT","wifi_pwd":"TTiVNIoT2023","line_no":"1102","wifi_endcoded":"False","lang":1,"org":"TG3","mqtt_user":"vnmqttlms","mqtt_pwd":"58KnaT3pE98k","mqtt_topic":"/TempControlVNLMS","mqtt_encoded":"False","mqtt_broker":"10.64.20.65","mqtt_port":1883,"DeviceName":"TEMP0022"}
)"; */

char* key = "abcdefghijklmnop";
Cipher *cipher = new Cipher();

void setup() {
    Serial.begin(115200);
    delay(1000);

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

    // Write data to a file
    /* File file = SD_MMC.open("/Jsondata.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
    } else {
        Serial.println("Writing to file...");
        file.println(jsonData);
        file.close();
        Serial.println("Write complete.");
    }
 */
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
        DynamicJsonDocument doc(2048);
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
     
        Serial.print("wifi_pwd: ");
        Serial.println(wifi_pwd);
        Serial.print("wifi_endcoded: ");
        Serial.println(wifi_encoded);

        Serial.print("mqtt_pwd: ");
        Serial.println(mqtt_pwd);
        Serial.print("mqtt_encoded: ");
        Serial.println(mqtt_encoded);

        if (wifi_encoded == "False" || mqtt_encoded == "False"){

            if (wifi_encoded == "False"){
                String wifi_pwd_encrypted = cipher->encryptString(wifi_pwd);
                doc["wifi_pwd"] = wifi_pwd_encrypted; 
                doc["wifi_encoded"] = "True";
            }

            if (mqtt_encoded == "False"){
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
                Serial.println("Writing modified JSON data to file...");
                file.println(newJsonData);
                file.close();
                Serial.println("Write complete.");
            }
        }

        String wifi_pwd_decrypted = cipher->decryptString(doc["wifi_pwd"]);
        String mqtt_pwd_decrypted = cipher->decryptString(doc["mqtt_pwd"]);

        Serial.println("Original String: wifi_pwd : TTiVNIoT2023");
        Serial.println("Original String: mqtt_pwd : 58KnaT3pE98k");

        Serial.println("Wifi_encoded: " + String(doc["wifi_encoded"].as<const char*>()));
        Serial.println("Mqtt_encoded: " + String(doc["mqtt_encoded"].as<const char*>()));

        Serial.println("Encrypted Wifi Pwd: " + String(doc["wifi_pwd"].as<const char*>()));
        Serial.println("Decrypted Wifi Pwd: " + wifi_pwd_decrypted);

        Serial.println("Encrypted mqtt Pwd: " + String(doc["mqtt_pwd"].as<const char*>()));
        Serial.println("Decrypted mqtt Pwd: " + mqtt_pwd_decrypted);

        Serial.println();
        Serial.println();
    }
}

void loop() {
    delay(10);
}
