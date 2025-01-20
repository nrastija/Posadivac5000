#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 26    // Digitalni pin za DHT senzor
#define DHTTYPE DHT11  // Tip DHT senzora

#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla

//Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT22 test!");
}

void loop() {
  //delay(2000);
  
  float temperatura_zraka = dht.readTemperature();
  float vlaznost_zraka = dht.readHumidity();

  int sensor_analog = analogRead(SOIL_MOISTURE_PIN);
  int _moisture = (sensor_analog / 4095.00) * 100;

  /*if (isnan(temperatura_zraka) || isnan(vlaznost_zraka)) {
    Serial.println("Greska pri čitanju vrijednosti sa DHT22 senzora!");
    return;
  }*/


  Serial.print("Moisture = ");
  Serial.print(_moisture); 
  Serial.println("%");
  delay(500);

  /*Serial.print("Temperatura: ");
  Serial.print(temperatura_zraka);
  Serial.println(" °C");

  Serial.print("Vlažnost: ");
  Serial.print(vlaznost_zraka);
  Serial.println(" %");

  delay(5000);*/
}

