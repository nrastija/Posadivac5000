#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>

class WifiHelper
{
public:
    WifiHelper(const char *ssid, const char *password);
    void connect();
    bool isConnected();
    
private:
    const char *wifiSSID;
    const char *wifiPassword;
};

#endif
