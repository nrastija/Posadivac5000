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
  // Provjera je li kartica prisutna
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Greška pri čitanju kartice!");
    return;
  }

  Serial.print("UID kartice: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // JSON podaci za zapis na karticu 
  // JSON podaci moraju odgovarati podacima na Azure remote bazi podataka kako bi prošli autentifikaciju za Posadivac5000!
  char jsonData[] = "{\"employee\":\"xxx\",\"code\":\"xxx\"}";

  //writeToCard(jsonData);
  //readFromCard();

  readAndParseJSON();

  mfrc522.PICC_HaltA();  // Završetak komunikacije s karticom
  mfrc522.PCD_StopCrypto1();
}

void writeToCard(char* data) {
  byte blockAddr = 4;  // Blok za pisanje podataka
  byte buffer[16]; // Buffer za pisanje podataka

  Serial.println("Pisanje JSON podataka na karticu...");

  for (int i = 0; i < strlen(data); i += 16) {
    memset(buffer, 0, sizeof(buffer));  
    strncpy((char*)buffer, data + i, 16);  

    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Greška pri autentifikaciji: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Greška pri pisanju podataka: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    Serial.print("Podaci zapisani u blok ");
    Serial.println(blockAddr);
    blockAddr++; // Sljedeći blok
  }
}

void readFromCard() {

}

void readAndParseJSON() {

} 

void parseJSON(String jsonString) {

}