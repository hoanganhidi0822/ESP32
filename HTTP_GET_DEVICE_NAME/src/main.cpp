#include <WiFi.h>
#include <HTTPClient.h>
 #include <Wire.h>
// Replace with your network credentials
const char* ssid = "TTIIoT";
const char* password = "TTiVNIoT2023";

// The URL of the server you want to connect to
const char* serverUrl = "http://10.147.37.59:5001/api/checkexist?key=XXV7lnIse9q4YGA11pXA&code=30BB7BBD4D77&devicetype=TEMP"; // Change to your server URL

void setup() {
  // Start the Serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Specify the URL
    http.begin(serverUrl);

    // Send the request
    int httpResponseCode = http.GET();

    // Check the returning code
    if (httpResponseCode > 0) {
      // Get the request response payload
      String payload = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(payload);
    } else {
      Serial.print("Error on sending GET: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Add a delay to avoid spamming the server
  delay(10000); // Delay in milliseconds
}