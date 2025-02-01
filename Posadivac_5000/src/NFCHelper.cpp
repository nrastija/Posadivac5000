#include "helpers/NfcHelper.h"

NFCHelper::NFCHelper(int ssPin, int rstPin) : mfrc522(ssPin, rstPin)
{
    SPI.begin();
    mfrc522.PCD_Init();
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }
}

bool NFCHelper::scanCard()
{
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        return false;
    }
    formatNFCID(mfrc522.uid.uidByte, mfrc522.uid.size);
    readAndParseJSON();
    return true;
}

NFCKartica NFCHelper::getCardData()
{
    return prislonjenaKartica;
}

void NFCHelper::formatNFCID(const unsigned char *uid, int uidLength)
{
    prislonjenaKartica.nfcid[0] = '\0';
    for (int i = 0; i < uidLength; i++)
    {
        char tempUID[4];
        sprintf(tempUID, "%02X", uid[i]);
        strcat(prislonjenaKartica.nfcid, tempUID);
        if (i < uidLength - 1)
        {
            strcat(prislonjenaKartica.nfcid, " ");
        }
    }
    Serial.print("Spremljeni UID string: ");
    Serial.println(prislonjenaKartica.nfcid);
}

void NFCHelper::readAndParseJSON()
{
    byte blockAddr = 4;
    byte buffer[18];
    byte size = sizeof(buffer);
    String jsonData = "";

    for (int i = 0; i < 3; i++)
    {
        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK)
        {
            Serial.print("Greška pri autentifikaciji: ");
            Serial.println(mfrc522.GetStatusCodeName(status));
            return;
        }

        status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
        if (status != MFRC522::STATUS_OK)
        {
            Serial.print("Greška pri čitanju podataka: ");
            Serial.println(mfrc522.GetStatusCodeName(status));
            return;
        }

        for (byte j = 0; j < 16; j++)
        {
            if (buffer[j] >= 32 && buffer[j] <= 126)
            {
                jsonData += (char)buffer[j];
            }
        }
        blockAddr++;
    }

    Serial.println("Pročitani podaci:" + jsonData);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error)
    {
        Serial.print("Greška pri parsiranju JSON-a: ");
        Serial.println(error.f_str());
        return;
    }

    strncpy(prislonjenaKartica.employee, doc["employee"], sizeof(prislonjenaKartica.employee) - 1);
    strncpy(prislonjenaKartica.code, doc["code"], sizeof(prislonjenaKartica.code));

    Serial.println("Podaci uspješno pročitani s kartice:");
    Serial.print("Zaposlenik: ");
    Serial.println(prislonjenaKartica.employee);
    Serial.print("Šifra: ");    
    Serial.println(prislonjenaKartica.code);
    Serial.println();
}
