#include "helpers/WifiHelper.h"
#include <Arduino.h>

WifiHelper::WifiHelper(const char *ssid, const char *password)
{
    wifiSSID = ssid;
    wifiPassword = password;
    WiFi.mode(WIFI_STA); 
    WiFi.begin(wifiSSID, wifiPassword);
}

void WifiHelper::connect()
{
    Serial.print("Povezivanje na WiFi mrežu: ");
    Serial.println(wifiSSID);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nSpojen na WiFi!");
    Serial.print("Lokalna IP adresa: ");
    Serial.println(WiFi.localIP());
}

bool WifiHelper::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
