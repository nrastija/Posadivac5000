#ifndef BLUETOOTH_HANDLER_H
#define BLUETOOTH_HANDLER_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// BLE UUID-ovi
// Version 4 UUID generirani na: https://www.uuidgenerator.net/
#define SERVICE_UUID        "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

class BluetoothHandler {
public:
    void initBLE();
    void sendSensorData(float temperatura, float vlaznost, int vlaga_tla);
    void startAdvertising();
};

#endif
