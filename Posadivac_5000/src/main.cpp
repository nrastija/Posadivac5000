#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

//Helper klase
#include "helpers/WifiHelper.h"
#include "helpers/BluetoothHelper.h"
#include "helpers/NFCHelper.h"
#include "helpers/DatabaseHelper.h"
#include "helpers/SensorHelper.h"

#define SERVICE_UUID "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

#define DHTPIN 26            // Digitalni pin za DHT senzor
#define DHTTYPE DHT11        // Tip DHT senzora
#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla (senzor vlage)

#define SS_PIN 5   // Pin za SDA (NFC)
#define RST_PIN 22 // Pin za RST (NFC)

// WiFi podaci
const char *ssid = "Nspot";
const char *password = "posadivac5000";

// Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

// Inicijalizacija NFC modula
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
bool deviceLogged = false;
bool employeeExists = false;

struct NFCKartica
{
    char nfcid[21];
    int userid;
    char employee[20];
    char code[8];
} prislonjenaKartica;

void checkDeviceReturn();

WifiHelper wifiHelper(ssid, password);
BluetoothHelper bluetoothHelper("ESP32-Posadivac5000", SERVICE_UUID, CHARACTERISTIC_UUID);
NFCHelper nfcHelper(SS_PIN, RST_PIN);
DatabaseHelper databaseHelper("https://posadivac5000-awayb2gthjbvhbga.northeurope-01.azurewebsites.net/esp_handler.php");
SensorHelper sensorHelper(DHTPIN, SOIL_MOISTURE_PIN);

void setup()
{
    Serial.begin(115200);
    Serial.println("================== SETUP ==================\n");
    dht.begin();

    SPI.begin();
    mfrc522.PCD_Init();

    bluetoothHelper.startAdvertising();

    wifiHelper.connect();
    Serial.println("===========================================\n\n");
}

void loop()
{
    if (!wifiHelper.isConnected())
    {
        Serial.println("WiFi veza izgubljena, pokušavam ponovno spojiti...");
        wifiHelper.connect();
    }

    deviceConnected = bluetoothHelper.isDeviceConnected();

    if (!deviceLogged)
    {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            Serial.println("================== NFC i BP ==================\n");
            Serial.println("NFC kartica prislonjena!");

            nfcHelper.formatNFCID(mfrc522.uid.uidByte, mfrc522.uid.size);
            nfcHelper.readAndParseJSON();
            databaseHelper.getUserDatafromDB(prislonjenaKartica, employeeExists);

            if (employeeExists)
            {
                Serial.println("Uređaj otključan!\n");
                databaseHelper.sendLogToDB(prislonjenaKartica.nfcid, prislonjenaKartica.userid, "UZIMANJE", "ACTIVE");
                deviceLogged = true;
            }
            else
            {
                Serial.println("Nepostojeći korisnik, uređaj zaključan!\n");
            }

            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            Serial.println("===========================================\n\n");
        }
        else
        {
            Serial.println("Uređaj zaključan, treba se otključati sa NFC karticom...");
        }

        delay(1000);
        return;
    }

    if (deviceLogged)
    {
        // Provjera vraća li korisnik uređaj prislanjanjem kartice
        if (deviceLogged)
        {
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
            {
                checkDeviceReturn();
            }
        }
    }

    if (deviceLogged && deviceConnected)
    {

        String data = sensorHelper.getSensorDataJSON();
        Serial.println("Podaci poslani: " + data);
        pCharacteristic->setValue(data.c_str());
        pCharacteristic->notify();

        delay(2000); // Pauza od 2 sekunde
    }
    else
    {

        Serial.println("Uređaj nije povezan sa Bluetooth LE, čekam vezu...");
        delay(5000); // Pauza dok čeka vezu
    }
}

void checkDeviceReturn()
{
    char tempUID[32] = "";

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        char temp[4];
        sprintf(temp, "%02X", mfrc522.uid.uidByte[i]); // Pretvorba bajta u hex string
        strcat(tempUID, temp);

        if (i < mfrc522.uid.size - 1)
        {
            strcat(tempUID, " "); // Dodavanje razmaka osim na zadnjem bajtu
        }
    }
    Serial.print("Usporedba UID-a: ");
    Serial.println(tempUID);

    // Provjera podudara li se UID kartice s onom koja je uzela uređaj
    if (strcmp(tempUID, prislonjenaKartica.nfcid) == 0)
    {
        Serial.println("Uređaj je vraćen!");
        databaseHelper.sendLogToDB(prislonjenaKartica.nfcid, prislonjenaKartica.userid, "VRACANJE", "COMPLETED");
        deviceLogged = false;
        Serial.println("Sustav ponovno zaključan.");
    }
    else
    {
        Serial.println("Kartica nije ista kao pri uzimanju uređaja!");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
 