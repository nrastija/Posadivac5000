#ifndef SENSOR_HELPER_H
#define SENSOR_HELPER_H

#include <DHT.h>

class SensorHelper
{
public:
    SensorHelper(int dhtPin, int soilMoisturePin);
    float readTemperature();
    float readHumidity();
    int readSoilMoisture();
    String getSensorDataJSON();

private:
    DHT dht;
    int soilMoisturePin;
};

#endif // SENSOR_HELPER_H
