#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

//Helper klase
#include "helpers/WifiHelper.h"
#include "helpers/BluetoothHelper.h"

#define SERVICE_UUID "676fa518-e4cb-4afa-aae4-f211fe532d48"
#define CHARACTERISTIC_UUID "a08ae7a0-11e8-483d-940c-a23d81245500"

#define DHTPIN 26            // Digitalni pin za DHT senzor
#define DHTTYPE DHT11        // Tip DHT senzora
#define SOIL_MOISTURE_PIN 35 // Analogni pin za mjerenje vlažnosti tla (senzor vlage)

#define SS_PIN 5   // Pin za SDA (NFC)
#define RST_PIN 22 // Pin za RST (NFC)

// WiFi podaci
const char *ssid = "Nspot";
const char *password = "posadivac5000";

// Inicijalizacija DHT senzora
DHT dht(DHTPIN, DHTTYPE);

// Inicijalizacija NFC modula
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
bool deviceLogged = false;
bool employeeExists = false;

struct NFCKartica
{
    char nfcid[21];
    int userid;
    char employee[20];
    char code[8];
} prislonjenaKartica;


void formatNFCID(const unsigned char *uid, int uidLength)
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

    Serial.println();
    Serial.print("Spremljeni UID string: ");
    Serial.println(prislonjenaKartica.nfcid);
}

void writeLogtoDB(bool uredajUzet);

void checkDeviceReturn()
{
    char tempUID[32] = ""; // Privremeni UID buffer za usporedbu

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        char temp[4];
        sprintf(temp, "%02X", mfrc522.uid.uidByte[i]); // Pretvorba bajta u hex string
        strcat(tempUID, temp);

        if (i < mfrc522.uid.size - 1)
        {
            strcat(tempUID, " "); // Dodavanje razmaka osim na zadnjem bajtu
        }
    }
    Serial.print("Usporedba UID-a: ");
    Serial.println(tempUID);

    // Provjera podudara li se UID kartice s onom koja je uzela uređaj
    if (strcmp(tempUID, prislonjenaKartica.nfcid) == 0)
    {
        Serial.println("Uređaj je vraćen!");
        writeLogtoDB(true);   // Spremi povratak uređaja u bazu
        deviceLogged = false; // Ponovno čeka na novi unos kartice
        Serial.println("Sustav ponovno zaključan.");
    }
    else
    {
        Serial.println("Kartica nije ista kao pri uzimanju uređaja!");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
 
void readAndParseJSON()
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

void getNFCData()
{
    String serverURL = "https://posadivac5000-awayb2gthjbvhbga.northeurope-01.azurewebsites.net/esp_handler.php";
    serverURL += "?employee=" + String(prislonjenaKartica.employee);
    serverURL += "&code=" + String(prislonjenaKartica.code);

    Serial.print("Šaljem GET zahtjev na: ");
    Serial.println(serverURL);

    HTTPClient http;
    http.begin(serverURL);             // Pokretanje HTTP zahtjeva
    int httpResponseCode = http.GET(); // Slanje GET zahtjeva

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP odgovor kod: ");
        Serial.println(httpResponseCode);

        String response = http.getString(); // Dohvaćanje odgovora
        Serial.println("Odgovor sa servera: " + response);

        // Parsiranje JSON odgovora
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("Greška pri parsiranju JSON-a: ");
            Serial.println(error.f_str());
            employeeExists = false;
        }

        else
        {
            const char *status = doc["status"];
            JsonArray users = doc["user"];

            if (strcmp(status, "success") == 0 && users.size() > 0)
            {
                Serial.println("Korisnik je pronađen u bazi.");
                employeeExists = true;

                // Ispis pronađenog korisnika
                prislonjenaKartica.userid = users[0]["id"]; // Pretpostavljamo da je 'id' broj
                strncpy(prislonjenaKartica.employee, users[0]["employee"], sizeof(prislonjenaKartica.employee) - 1);
                strncpy(prislonjenaKartica.code, users[0]["code"], sizeof(prislonjenaKartica.code) - 1);

                // Osiguraj završetak stringova
                prislonjenaKartica.employee[sizeof(prislonjenaKartica.employee) - 1] = '\0';
                prislonjenaKartica.code[sizeof(prislonjenaKartica.code) - 1] = '\0';

                Serial.print("Korisnik ID: ");
                Serial.println(prislonjenaKartica.userid);
                Serial.print("Korisnik: ");
                Serial.println(prislonjenaKartica.employee);
                Serial.print("Kod: ");
                Serial.println(prislonjenaKartica.code);
            }
            else
            {
                Serial.println("Korisnik nije pronađen.");
                employeeExists = false;
            }
        }
    }

    http.end();
}

void writeLogtoDB(bool uredajUzet)
{
    Serial.println("Zapisivanje u bazu podataka...");

    HTTPClient http;

    String serverUrl = "https://posadivac5000-awayb2gthjbvhbga.northeurope-01.azurewebsites.net/esp_handler.php";

    String actionType = uredajUzet ? "VRACANJE" : "UZIMANJE"; // False -> uredaj se tek uzima
    String status = uredajUzet ? "COMPLETED" : "ACTIVE";

    String postData = "nfc_uid=" + String(prislonjenaKartica.nfcid) +
                      "&user_id=" + String(prislonjenaKartica.userid) +
                      "&action_type=" + actionType +
                      "&status=" + status;

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP odgovor kod: ");
        Serial.println(httpResponseCode);

        String response = http.getString();
        Serial.println("Odgovor sa servera: " + response);

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("Greška pri parsiranju odgovora: ");
            Serial.println(error.f_str());
            return;
        }

        const char *status = doc["status"];
        const char *message = doc["message"];

        if (strcmp(status, "success") == 0)
        {
            Serial.println("Podaci uspješno spremljeni!");
        }
        else
        {
            Serial.print("Greška: ");
            Serial.println(message);
        }
    }
    else
    {
        Serial.print("Greška pri slanju POST zahtjeva. HTTP kod: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}


WifiHelper wifiHelper(ssid, password);
BluetoothHelper bluetoothHelper(SERVICE_UUID, CHARACTERISTIC_UUID);

void setup()
{
    Serial.begin(115200);
    Serial.println("================== SETUP ==================\n");
    dht.begin();

    SPI.begin();
    mfrc522.PCD_Init();

    bluetoothHelper.startAdvertising();

    wifiHelper.connect();
    Serial.println("===========================================\n\n");
}

void loop()
{
    deviceConnected = bluetoothHelper.isDeviceConnected();

    if (!deviceLogged)
    {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            Serial.println("================== NFC i BP ==================\n");
            Serial.println("NFC kartica prislonjena!");

            formatNFCID(mfrc522.uid.uidByte, mfrc522.uid.size);
            readAndParseJSON();
            getNFCData();

            if (employeeExists)
            {
                Serial.println("Uređaj otključan!\n");
                writeLogtoDB(false);
                deviceLogged = true;
            }
            else
            {
                Serial.println("Nepostojeći korisnik, uređaj zaključan!\n");
            }

            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            Serial.println("===========================================\n\n");
        }
        else
        {
            Serial.println("Uređaj zaključan, treba se otključati sa NFC karticom...");
        }

        delay(1000);
        return;
    }

    if (deviceLogged)
    {
        // Provjera vraća li korisnik uređaj prislanjanjem kartice
        if (deviceLogged)
        {
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
            {
                checkDeviceReturn();
            }
        }
    }

    if (deviceLogged && deviceConnected)
    {

        float temperatura_zraka = dht.readTemperature();
        float vlaznost_zraka = dht.readHumidity();
        int sensor_analog = analogRead(SOIL_MOISTURE_PIN);
        int vlaznost_tla = (sensor_analog / 4095.0) * 100; // Normalizacija vlage tla

        if (isnan(temperatura_zraka) || isnan(vlaznost_zraka))
        {
            Serial.println("Greska pri citanju vrijednosti sa DHT22 senzora!");
            return;
        }

        // Kreiranje JSON stringa s podacima
        String data = "{";
        data += "\"temperature\":" + String(temperatura_zraka, 1) + ",";
        data += "\"humidity\":" + String(vlaznost_zraka, 1) + ",";
        data += "\"soil_moisture\":" + String(vlaznost_tla);
        data += "}";

        Serial.println("Podaci poslani: " + data);
        pCharacteristic->setValue(data.c_str());
        pCharacteristic->notify();

        delay(2000); // Pauza od 2 sekunde
    }
    else
    {

        Serial.println("Uređaj nije povezan sa Bluetooth LE, čekam vezu...");
        delay(5000); // Pauza dok čeka vezu
    }
}
