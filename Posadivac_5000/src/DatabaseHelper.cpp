#include "helpers/DatabaseHelper.h"
#include <Arduino.h>

DatabaseHelper::DatabaseHelper(String serverUrl)
{
    this->serverUrl = serverUrl;
}

bool DatabaseHelper::sendLogToDB(String nfcUID, int userID, String actionType, String status)
{
    HTTPClient http;
    String postData = "nfc_uid=" + nfcUID +
                      "&user_id=" + String(userID) +
                      "&action_type=" + actionType +
                      "&status=" + status;

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP odgovor kod: ");
        Serial.println(httpResponseCode);
        return true;
    }
    else
    {
        Serial.print("Greška pri slanju POST zahtjeva. HTTP kod: ");
        Serial.println(httpResponseCode);
        return false;
    }

    http.end();
}

bool DatabaseHelper::getUserDatafromDB(NFCKartica &kartica, bool &employeeExists)
{
    String requestURL = serverUrl + "?employee=" + String(kartica.employee) + "&code=" + String(kartica.code);

    Serial.print("Šaljem GET zahtjev na: ");
    Serial.println(requestURL);

    HTTPClient http;
    http.begin(requestURL);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP odgovor kod: ");
        Serial.println(httpResponseCode);

        String response = http.getString();
        Serial.println("Odgovor sa servera: " + response);

        // Parsiranje JSON odgovora
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("Greška pri parsiranju JSON-a: ");
            Serial.println(error.f_str());
            employeeExists = false;
            return false;
        }

        const char *status = doc["status"];
        JsonArray users = doc["user"];

        if (strcmp(status, "success") == 0 && users.size() > 0)
        {
            Serial.println("Korisnik je pronađen u bazi.");
            employeeExists = true;

            kartica.userid = users[0]["id"];
            strncpy(kartica.employee, users[0]["employee"], sizeof(kartica.employee) - 1);
            strncpy(kartica.code, users[0]["code"], sizeof(kartica.code) - 1);

            kartica.employee[sizeof(kartica.employee) - 1] = '\0';
            kartica.code[sizeof(kartica.code) - 1] = '\0';

            Serial.print("Korisnik ID: ");
            Serial.println(kartica.userid);
            Serial.print("Korisnik: ");
            Serial.println(kartica.employee);
            Serial.print("Kod: ");
            Serial.println(kartica.code);
            return true;
        }
        else
        {
            Serial.println("Korisnik nije pronađen.");
            employeeExists = false;
            return false;
        }
    }
    else
    {
        Serial.print("Greška pri slanju GET zahtjeva. HTTP kod: ");
        Serial.println(httpResponseCode);
        employeeExists = false;
        return false;
    }

    http.end();
}
