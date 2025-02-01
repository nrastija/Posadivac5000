#ifndef BLUETOOTH_HELPER_H
#define BLUETOOTH_HELPER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

class BluetoothHelper
{
public:
    BluetoothHelper(const char *deviceName, const char *serviceUUID, const char *characteristicUUID);
    void startAdvertising();
    bool isDeviceConnected();

private:
    BLEServer *pServer;
    BLECharacteristic *pCharacteristic;
    bool deviceConnected;

    class MyServerCallbacks : public BLEServerCallbacks
    {
        BluetoothHelper *bluetoothHelper;

    public:
        MyServerCallbacks(BluetoothHelper *helper) : bluetoothHelper(helper) {}
        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
    };
};

#endif 
