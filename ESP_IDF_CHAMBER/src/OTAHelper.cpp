#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "OTAHelper.h"

#define BUFFERSIZE 4000
WiFiClient netOTA;

String vNewVersion = "N";

int _totalLength;
int _currentLength = 0; // current size of written firmware

String _firmwareQuery = String(APPAPI) + "/firmware?filePrefix=" + String(APPUPDNAME) + "&screenSize=" + String(APPSCREENSIZE) + "&version=" + String(APPVERSION);

String OTACheck(boolean forceUpdate)
{
    HTTPClient client;
    client.addHeader("X-Secret-Key", String(APPAPIKEY));
    JsonDocument doc;

    String queryURL = _firmwareQuery + "&update=N";
    Serial.println(queryURL);
    client.begin(netOTA, queryURL.c_str());

    int httpResponseCode = client.GET();

    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = client.getString();
        Serial.println(payload);
        client.end();
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return "N";
        }

        vNewVersion = doc["hasnewversion"].as<String>();
    }
    else
    {
        client.end();
    }

    if (vNewVersion == "Y" && forceUpdate)
    {
        OTAUpdate();
    }

    return vNewVersion;
}

void OTAUpdate()
{
    // Connect to external web server
    HTTPClient client;
    client.addHeader("X-Secret-Key", String(APPAPIKEY));
    int loopNumber = 0;
    Serial.println("Checking if new firmware is available.");

    String fwVersionURL = _firmwareQuery + "&update=Y";

    Serial.println(fwVersionURL);

    client.begin(netOTA, fwVersionURL.c_str());
    // Get file, just to check if each reachable
    int resp = client.GET();
    Serial.print("Response: ");
    Serial.println(resp);
    // If file is reachable, start downloading
    if (resp == 200)
    {
        _totalLength = client.getSize();
        // transfer to local variable
        int len = _totalLength;
        // this is required to start firmware update process
        Update.begin(UPDATE_SIZE_UNKNOWN);
        Serial.printf("FW Size: %u\n", _totalLength);
        // create buffer for read
        uint8_t buff[BUFFERSIZE] = {0};
        // get tcp stream
        WiFiClient *stream = client.getStreamPtr();
        // read all data from server
        Serial.println("Updating firmware...");
        while (client.connected() && (len > 0 || len == -1))
        {

            if (loopNumber % 180 == 0)
            {
                Serial.print(".");
            }
            loopNumber++;

            // get available data size
            size_t size = stream->available();
            if (size)
            {
                // read up to 128 byte
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                // pass to function
                updateFirmware(buff, c);
                if (len > 0)
                {
                    len -= c;
                }
                Serial.printf("Downloading Stream: %d \n", _currentLength);
            }
            // delay(1);
        }
    }
    else
    {
        Serial.println("Cannot download firmware file.");
    }
    client.end();
}

void updateFirmware(uint8_t *data, size_t len)
{
    Update.write(data, len);
    _currentLength += len;

    if (_currentLength != _totalLength)
        return;
    Update.end(true);
    Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", _currentLength);
    // Restart ESP32 to see changes
    ESP.restart();
}