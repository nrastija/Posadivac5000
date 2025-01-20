#include <Arduino.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

//Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("DHT22 test!");
}

void loop() {
  delay(2000);
  
  float temperatura_zraka = dht.readTemperature();
  float vlaznost_zraka = dht.readHumidity();

  if (isnan(temperatura_zraka) || isnan(vlaznost_zraka)) {
    Serial.println("Greska pri čitanju vrijednosti sa DHT22 senzora!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura_zraka);
  Serial.println(" °C");

  Serial.print("Vlažnost: ");
  Serial.print(vlaznost_zraka);
  Serial.println(" %");

}

