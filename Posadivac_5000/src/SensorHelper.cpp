#include "helpers/SensorHelper.h"

SensorHelper::SensorHelper(int dhtPin, int soilMoisturePin) : dht(dhtPin, DHT11)
{
    dht.begin();
    this->soilMoisturePin = soilMoisturePin;
}

float SensorHelper::readTemperature()
{
    return dht.readTemperature();
}

float SensorHelper::readHumidity()
{
    return dht.readHumidity();
}

int SensorHelper::readSoilMoisture()
{
    return analogRead(soilMoisturePin) * 100 / 4095; // Normalizacija vlage tla
}

String SensorHelper::getSensorDataJSON()
{
    float temperatura_zraka = readTemperature();
    float vlaznost_zraka = readHumidity();
    int vlaznost_tla = readSoilMoisture();

    if (isnan(temperatura_zraka) || isnan(vlaznost_zraka))
    {
        return "{\"error\": \"Greška pri čitanju senzora\"}";
    }

    String json = "{";
    json += "\"temperature\":" + String(temperatura_zraka, 1) + ",";
    json += "\"humidity\":" + String(vlaznost_zraka, 1) + ",";
    json += "\"soil_moisture\":" + String(vlaznost_tla);
    json += "}";

    return json;
}
