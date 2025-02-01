#ifndef DATABASE_HELPER_H
#define DATABASE_HELPER_H

#include <HTTPClient.h>
#include <ArduinoJson.h>

struct NFCKartica
{
    char nfcid[21];
    int userid;
    char employee[20];
    char code[8];
};

class DatabaseHelper
{
public:
    DatabaseHelper(String serverUrl);
    bool sendLogToDB(String nfcUID, int userID, String actionType, String status);
    bool getUserDatafromDB(NFCKartica &kartica, bool &employeeExists);

private:
    String serverUrl;
};

#endif // DATABASE_HELPER_H
