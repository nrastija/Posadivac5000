#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Electroniccats_PN7150.h"


#define SERVICE_UUID        "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

#define DHTPIN 26    // Digitalni pin za DHT senzor
#define DHTTYPE DHT11  // Tip DHT senzora
#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla


//Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

//Inicijalizacija NFC modula
Electroniccats_PN7150 nfc(33, 32, 0x28);

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
bool deviceLogged = false;
bool userExists = false;

struct NFCKartica {
    char nfcid[21];
    char user[20];
    char pass[8];
} prislonjenaKartica;


class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("Uređaj spojen!");
    };

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        Serial.println("Uređaj odspojen!");
        BLEDevice::startAdvertising();  // Ponovno pokreni oglašavanje
    }
};

void formatNFCID(const unsigned char* uid, int uidLength) {
    char tempUID[3];  // Privremeni buffer za svaki bajt

    prislonjenaKartica.nfcid[0] = '\0';  // Osiguraj da je string prazan prije dodavanja

    for (int i = 0; i < uidLength; i++) {
        sprintf(tempUID, "%02X", uid[i]);  // Konverzija u heks string (dva znaka po bajtu)
        strcat(prislonjenaKartica.nfcid, tempUID);  // Dodaj bajt u konačni string
    }

    Serial.print("Spremljeni UID string: ");
    Serial.println(prislonjenaKartica.nfcid);
}



void setup() {
    Serial.begin(115200);
    dht.begin();
    Serial.println("DHT22 test!");

    // Inicijalizacija BLE
    BLEDevice::init("ESP32-Posadivac5000");
    BLEDevice::setMTU(512);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks()); // Postavljanje callbacka

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    pCharacteristic->setValue("Cekam podatke...");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    BLEDevice::startAdvertising();

    Serial.println("BLE oglasavanje pokrenuto...");

    // Inicijalizacija NFC
    Serial.print("connectNCI=");Serial.println(nfc.connectNCI());
    Serial.print("configureSettings=");Serial.println(nfc.configureSettings());
    Serial.print("configMode=");Serial.println(nfc.configMode());
    Serial.print("startDiscovery=");Serial.println(nfc.startDiscovery());

    Serial.println("NFC uspješno inicijaliziran!");
}


void loop() {
    if (!deviceLogged) {
        if (nfc.isTagDetected()) {
            const unsigned char* uid = nfc.remoteDevice.getNFCID();
            int uidLength = nfc.remoteDevice.getNFCIDLen();
            Serial.println("NFC kartica prislonjena!");

            formatNFCID(uid, uidLength);

            getNFCData();

            if (userExists) {
                writeLogtoDB(true);
                deviceLogged = true;

                Serial.println("Uređaj otključan!");
            } else {
                Serial.println("Nepostojeći korisnik, uređaj zaključan!");
            }
        } else{
            Serial.println("Uređaj zaključan, treba se otključati sa NFC karticom...");
            
        }
        delay(1000);
        return;
    } 

    if (deviceLogged && deviceConnected){ 

        float temperatura_zraka = dht.readTemperature();
        float vlaznost_zraka = dht.readHumidity();
        int sensor_analog = analogRead(SOIL_MOISTURE_PIN);
        int vlaznost_tla = (sensor_analog / 4095.0) * 100;  // Normalizacija vlage tla

        if (isnan(temperatura_zraka) || isnan(vlaznost_zraka)) {
            Serial.println("Greska pri citanju vrijednosti sa DHT22 senzora!");
            return;
        }

        // Kreiranje JSON stringa s podacima
        String data = "{";
        data += "\"temperature\":" + String(temperatura_zraka, 1) + ",";
        data += "\"humidity\":" + String(vlaznost_zraka, 1) + ",";
        data += "\"soil_moisture\":" + String(vlaznost_tla);
        data += "}";

        Serial.println("Podaci poslani: " + data);
        pCharacteristic->setValue(data.c_str());
        pCharacteristic->notify();

        delay(2000);  // Pauza od 2 sekunde
    } else {
        Serial.println("Uređaj nije povezan sa Bluetooth LE, čekam vezu...");
        delay(5000);  // Pauza dok čeka vezu
    }
    
    
}

