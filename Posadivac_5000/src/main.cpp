#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <NimBLEDevice.h>

// Version 4 UUID generirani na: https://www.uuidgenerator.net/
#define SERVICE_UUID        "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

#define DHTPIN 26    // Digitalni pin za DHT senzor
#define DHTTYPE DHT11  // Tip DHT senzora

#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla


// Deklaracija globalnih BLE varijabli
NimBLEServer* pServer = nullptr; 
NimBLECharacteristic* pCharacteristic;

//Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);


// Funkcija za inicijalizaciju BLE
void setupBLE() { 
    NimBLEDevice::init("ESP32-Posadivac5000");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
}

// Kreiranje BLE servisa i karakteristike
void createService() {
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pCharacteristic->setValue("Čekam...");
    pService->start(); 
}

// Funkcija za početak oglašavanja
void startAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("ESP32-Posadivac5000");
    pAdvertising->start();
    Serial.println("Advertising započet...");
}

// Callback klasa za praćenje povezivanja uređaja
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Uređaj spojen: %s\n", connInfo.getAddress().toString().c_str());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("Uređaj odspojen, advertising reset\n");
        NimBLEDevice::startAdvertising();
    }
};


void sendSensorData(float temperatura, float vlaznost, int vlaga_tla) {
    if (NimBLEDevice::getServer()->getConnectedCount() > 0) {
        String data = "{";
        data += "\"temperature\":" + String(temperatura, 1) + ",";
        data += "\"humidity\":" + String(vlaznost, 1) + ",";
        data += "\"soil_moisture\":" + String(vlaga_tla);
        data += "}";

        pCharacteristic->setValue(data.c_str());
        pCharacteristic->notify();
        Serial.println("Podaci poslani: " + data);
    }
}


void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT22 test!");

  setupBLE();         // Inicijalizacija BLE
  createService();    // Kreiranje servisa i karakteristika
  startAdvertising(); // Početak BLE reklamiranja

}

void loop() {
  
  float temperatura_zraka = dht.readTemperature();
  float vlaznost_zraka = dht.readHumidity();

  int sensor_analog = analogRead(SOIL_MOISTURE_PIN);
  int vlaznost_tla = (sensor_analog / 4095.00) * 100;

  if (isnan(temperatura_zraka) || isnan(vlaznost_zraka)) {
    Serial.println("Greska pri čitanju vrijednosti sa DHT22 senzora!");
    return;
  }

  sendSensorData(temperatura_zraka, vlaznost_zraka, vlaznost_tla);

  delay(5000);
}

