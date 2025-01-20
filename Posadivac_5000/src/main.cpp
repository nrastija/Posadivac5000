#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <NimBLEDevice.h>
#include "BluetoothHandler.h"

#define SERVICE_UUID        "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

#define DHTPIN 26    // Digitalni pin za DHT senzor
#define DHTTYPE DHT11  // Tip DHT senzora

#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla

//Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

BluetoothHandler btHandler; // Inicijalizacija objekta za upravljanje BLE

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT22 test!");

  btHandler.initBLE(); // Inicijalizacija BLE
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

  btHandler.sendSensorData(temperatura_zraka, vlaznost_zraka, vlaznost_tla);

  Serial.print("Temperatura: ");
  Serial.print(temperatura_zraka);
  Serial.println(" °C");

  Serial.print("Vlažnost zraka: ");
  Serial.print(vlaznost_zraka);
  Serial.println(" %");

  Serial.print("Vlažnost tla: ");
  Serial.print(vlaznost_tla);
  Serial.println(" %");

  delay(5000);
}

