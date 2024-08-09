#include <Arduino.h>
#include "Cipher.h"

char* key = "abcdefghijklmnop";
Cipher *cipher = new Cipher();

void setup() {
  Serial.begin(115200);
  
  cipher->setKey(key);

  String data = "ESP32 AES128bit Encryption example";
  String cipherString = cipher->encryptString(data);
  String decipheredString = cipher->decryptString(cipherString);

  Serial.println("Original String: " + data);
  Serial.println("Encrypted String: " + cipherString);
  Serial.println("Decrypted String: " + decipheredString);
}

void loop() {
  // put your main code here, to run repeatedly:
}
