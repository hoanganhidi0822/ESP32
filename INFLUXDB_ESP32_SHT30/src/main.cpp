
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"


#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>
#include "SHT85.h"  // Make sure this library is installed

#define SHT30_ADDRESS  0x44
#define WIFI_SSID "TTIIoT"
#define WIFI_PASSWORD "TTiVNIoT2023"
#define INFLUXDB_URL "http://10.147.37.59:8086"
#define INFLUXDB_TOKEN "HryhWSaViYzTamTNUA6SZBshr0KEzina1pXFbeGkyhU4ynrtDFdbtyakkaazpz2iPNklpgkL6XMFL3OncN2nOw=="
#define INFLUXDB_ORG "00ab82315618550f"
#define INFLUXDB_BUCKET "HoangAnhESP32"
#define TZ_INFO "UTC7"

SHT30 sht(SHT30_ADDRESS);
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("sensor_data");

unsigned long lastReadTime = 0;
const unsigned long interval = 2000;  // Reading interval in milliseconds
float High_temp = 28;
float Low_temp = 22;
float High_humid = 80;
float Low_humid = 50;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Setup wifi
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to wifi");
    while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
    Serial.println();

    // Sync time
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check InfluxDB connection
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }

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

void loop() {
    unsigned long currentTime = millis();

    // Read temperature and humidity every 2 seconds
    if (currentTime - lastReadTime >= interval) {
        lastReadTime = currentTime;

        // Read temperature and humidity
        uint32_t start = micros();
        sht.read();
        uint32_t stop = micros();

        float temperature = sht.getTemperature();
        float humidity = sht.getHumidity();

        if (!isnan(temperature) && !isnan(humidity)) {
            // Print temperature and humidity
            Serial.print("Temperature: ");
            Serial.print(temperature);
            Serial.print(" °C, Humidity: ");
            Serial.print(humidity);
            Serial.println(" %");

            // Print the time taken for the read operation
            Serial.print("Time taken: ");
            Serial.print((stop - start) * 0.001);
            Serial.println(" ms");

            // Prepare data point
            sensor.clearFields();  // Clear previous fields
            sensor.addField("Temperature", temperature);
            sensor.addField("Humidity", humidity);
            sensor.addField("High temp", High_temp);
            sensor.addField("Low temp", Low_temp);
            sensor.addField("High humid", High_humid);
            sensor.addField("Low humid", Low_humid);
            

            // Write point
            if (!client.writePoint(sensor)) {
                Serial.print("InfluxDB write failed: ");
                Serial.println(client.getLastErrorMessage());
            } else {
                Serial.println("Data written to InfluxDB");
            }
        } else {
            Serial.println("Failed to read from SHT30 sensor!");
        }
    }
}
