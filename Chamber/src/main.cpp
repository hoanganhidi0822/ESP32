#include <HardwareSerial.h>
#include <SPI.h>
HardwareSerial ModbusSerial(2);  // Use UART2

// Pin definitions
#define MAX485_DE 18
#define MAX485_RE 19

void setup() {
  // initialize Modbus ASCII communication
  Serial.begin(115200);
  ModbusSerial.begin(9600, SERIAL_8N1, 16, 17);
  
  // set DE/RE pins as output
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);

  // initially set DE/RE pins low
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE, 0);
}

void sendModbusASCII(String message) {
  // Enable transmit mode
  digitalWrite(MAX485_DE, 1);
  digitalWrite(MAX485_RE, 1);
  
  ModbusSerial.print(':');  // Start character
  ModbusSerial.print(message);
  ModbusSerial.print("\r\n");  // End characters

  // Wait for transmission to complete
  ModbusSerial.flush();

  // Enable receive mode
  digitalWrite(MAX485_DE, 0);
  digitalWrite(MAX485_RE, 0);
}

String readModbusASCII() {
  String response = "";
  while (ModbusSerial.available()) {
    char c = ModbusSerial.read();
    response += c;
  }
  return response;
}

void loop() {
  String request = "010300000002FA";
  sendModbusASCII(request);

  delay(1000);  // wait for a response

  String response = readModbusASCII();
  Serial.println("Response: " + response);

  // Process the response here
}
