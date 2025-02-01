#ifndef NFC_HELPER_H
#define NFC_HELPER_H

#include <Arduino.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

struct NFCKartica
{
    char nfcid[21];
    int userid;
    char employee[20];
    char code[8];
};

class NFCHelper
{
public:
    NFCHelper(int ssPin, int rstPin);
    bool scanCard();
    NFCKartica getCardData();
    void formatNFCID(const unsigned char *uid, int uidLength);
    void readAndParseJSON();
    
private:
    MFRC522 mfrc522;
    MFRC522::MIFARE_Key key;
    NFCKartica prislonjenaKartica;
};

#endif // NFC_HELPER_H
