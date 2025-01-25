#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

#define SS_PIN 5      // Pin za SDA (prilagodi prema svojoj ploči)
#define RST_PIN 22    // Pin za RST (prilagodi prema svojoj ploči)

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Inicijalizacija RC522 modula

MFRC522::MIFARE_Key key;  // MIFARE ključ

void writeToCard(char* data);
void readFromCard();
void readAndParseJSON();
void parseJSON(String jsonString);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  // default kljuc (0xFFFFFFFFFFFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("Prislonite karticu...");
}

void loop() {
}

void writeToCard(char* data) {

}

void readFromCard() {

}

void readAndParseJSON() {

} 

void parseJSON(String jsonString) {

}