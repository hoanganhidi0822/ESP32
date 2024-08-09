/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <SPI.h> 
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include "SHT85.h"

#define SHT30_ADDRESS  0x44

char boardID[23];

// Replace the next variables with your SSID/Password combination
const char* ssid = "TTIIoT";
const char* password = "TTiVNIoT2023";

// Add your MQTT Broker IP address, example:
// const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "10.147.37.59";

// MQTT username and password
const char* mqtt_username = "hoanganh";
const char* mqtt_password = "123";

WiFiClient espClient;
PubSubClient client(espClient);
SHT30 sht(SHT30_ADDRESS);

long lastMsg = 0;
char msg[50];
int value = 0;

const int ledPin = 2; // Define the GPIO pin for the LED

uint32_t start;
uint32_t stop;

// Function declarations
void setup_wifi();
void callback(char* topic, byte* message, unsigned int length);
void reconnect();

void setup() {
  pinMode(ledPin, OUTPUT); // Initialize the GPIO pin as an output
  Serial.begin(115200);
  snprintf(boardID, 23, "%llX", ESP.getEfuseMac());
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

  // If a message is received on the topic HoangAnh/test, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "HoangAnh/test") {
    Serial.print("Changing output to ");
    if (messageTemp == "on") {
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    } else if (messageTemp == "off") {
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
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
      snprintf(msg, 50, "Temperature: %.2f C, Humidity: %.2f %%", temperature, humidity);
      Serial.print("Publishing: ");
      Serial.println(msg);
      client.publish("HoangAnh/test", msg);
    } else {
      Serial.println("Failed to read from SHT30 sensor!");
    }

    // Print the time taken for the read operation
    Serial.print("\tTime taken: ");
    Serial.print((stop - start) * 0.001);
    Serial.println(" ms");
  }
}
