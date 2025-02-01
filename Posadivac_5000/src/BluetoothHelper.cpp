#include <Arduino.h>
#include "helpers/BluetoothHelper.h"

BluetoothHelper::BluetoothHelper(const char *deviceName, const char *serviceUUID, const char *characteristicUUID)
{
    BLEDevice::init(deviceName);
    BLEDevice::setMTU(512);
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(this));

    BLEService *pService = pServer->createService(serviceUUID);

    pCharacteristic = pService->createCharacteristic(
        characteristicUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    pService->start();
    BLEDevice::startAdvertising();

    Serial.println("BLE oglašavanje pokrenuto...");
}

void BluetoothHelper::startAdvertising()
{
    BLEDevice::startAdvertising();
}

bool BluetoothHelper::isDeviceConnected()
{
    return deviceConnected;
}

void BluetoothHelper::MyServerCallbacks::onConnect(BLEServer *pServer)
{
    bluetoothHelper->deviceConnected = true;
    Serial.println("Uređaj spojen!");
}

void BluetoothHelper::MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    bluetoothHelper->deviceConnected = false;
    Serial.println("Uređaj odspojen!");
    BLEDevice::startAdvertising();
}
