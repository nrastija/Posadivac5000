#include "BluetoothHandler.h"
#include <Arduino.h>

// Globalne varijable za BLE server i karakteristiku
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;

// Callback klasa za praćenje BLE događaja
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Uređaj povezan: %s\n", connInfo.getAddress().toString().c_str());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.println("Uređaj odspojen, pokretanje advertisinga...");
        NimBLEDevice::startAdvertising();
    }
};

// Inicijalizacija BLE servera
void BluetoothHandler::initBLE() {
    NimBLEDevice::init("ESP32-Posadivac5000");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pCharacteristic->setValue("Čekam podatke...");
    pService->start();
    startAdvertising();
}

// Funkcija za slanje podataka putem BLE
void BluetoothHandler::sendSensorData(float temperatura, float vlaznost, int vlaga_tla) {
    if (pServer->getConnectedCount() > 0) {
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

// Funkcija za pokretanje BLE advertisinga
void BluetoothHandler::startAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("ESP32-Posadivac5000");
    pAdvertising->start();
    Serial.println("BLE reklamiranje započeto...");
}
