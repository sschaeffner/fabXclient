#include "backend.h"
#include <HTTPClient.h>
#include <base64.h>

void Backend::begin() {
    // read device mac into String
    byte mac[6];
    WiFi.macAddress(mac);
    char macBuffer[13];
    sprintf(macBuffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    deviceMac = String(macBuffer);

    hc.setReuse(true);
    hc.setTimeout(1000);
    hc.setConnectTimeout(2000);
}

bool Backend::readConfig(Config &config, bool allowCached) {
    String url = config.backendUrl + deviceMac + "/config";

    Serial.print("Backend::readConfig Url=");
    Serial.println(url);

    hc.begin(url);

    String auth = base64::encode(deviceMac + ":" + secret);
    Serial.printf("Backend::readConfig secret=%s\n", secret.c_str());
    Serial.printf("Backend::readConfig auth=%s\n", auth.c_str());
    hc.addHeader("Authorization", "Basic " + auth);

    int httpCode = hc.GET();

    Serial.printf("Backend::readConfig httpCode=%i\n", httpCode);

    char configBuffer[512];

    if (httpCode == 200) {
        String payload = hc.getString();
        
        strncpy(configBuffer, payload.c_str(), 512);
        configBuffer[511] = '\0';

        Serial.printf("Backend::readConfig HTTP Code %i\n", httpCode);
        Serial.printf("configBuffer:\n%s", configBuffer);

        Serial.printf("writing config to SD card...\n");
        
        File configFile = SD.open("/config.txt", "w");
        if (configFile) {
            configFile.printf(configBuffer);
            configFile.close();
            Serial.printf("done writing config to SD card\n");
        } else {
            Serial.printf("could not open /config.txt on SD card\n");
        }
    } else {
        if (!allowCached) return false;

        File configFile = SD.open("/config.txt", "r");
        if (configFile) {
            String payload = configFile.readString();
            Serial.printf("Backend::readConfig from SD card\n");

            strncpy(configBuffer, payload.c_str(), 512);
            configBuffer[511] = '\0';

            Serial.printf("configBuffer:\n%s", configBuffer);

            configFile.close();
        } else {
            Serial.printf("could not open /config.txt on SD card\n");
            return false;
        }
    }

    char lineDelimiter[] = "\n";
    char *linePtr;
    char *lineSavePtr;

    linePtr = strtok_r(configBuffer, lineDelimiter, &lineSavePtr);
    int lineNr = 0;

    char commaDelimiter[] = ",";
    char *toolPtr;
    char *toolSavePtr;
    int toolNr = 0;
    int toolPNr = 0;

    Serial.println("=== CONFIG ===");

    while (linePtr != NULL) {
        if (lineNr == 0) {
            config.deviceName += linePtr;
            Serial.printf("Device Name: %s\n", config.deviceName.c_str());
        } else if (lineNr == 1) {
            config.bgImageUrl += linePtr;
            Serial.printf("Device BG image url: %s\n", config.bgImageUrl.c_str());
        } else if (lineNr == 2) {
            config.backupBackendUrl += linePtr;
            Serial.printf("Device backup backend url: %s\n", config.backupBackendUrl.c_str());
        } else {
            //tool configuration: tool_id,pin_id,tool_type,tool_name
            toolPtr = strtok_r(linePtr, commaDelimiter, &toolSavePtr);

            toolPNr = 0;
            while (toolPtr != NULL) {
                switch (toolPNr) {
                    case 0:
                        Serial.printf("toolId: %s\n", toolPtr);
                        config.toolIds[toolNr] = atoi(toolPtr);
                        break;
                    case 1:
                        Serial.printf("- pinId: %s\n", toolPtr);
                        config.toolPins[toolNr] = atoi(toolPtr);
                        break;
                    case 2:
                        Serial.print("- toolType: ");
                        if (strcmp(toolPtr, "UNLOCK") == 0) {
                            config.toolModes[toolNr] = UNLOCK;
                            Serial.println("UNLOCK");
                        } else if (strcmp(toolPtr, "KEEP") == 0) {
                            config.toolModes[toolNr] = KEEP;
                            Serial.println("KEEP");
                        }
                        break;
                    case 3:
                        Serial.printf("- toolName: %s\n", toolPtr);
                        config.toolNames[toolNr] += toolPtr;
                        break;
                }

                ++toolPNr;
                ++config.toolAmount;
                toolPtr = strtok_r(NULL, commaDelimiter, &toolSavePtr);
            }

            ++toolNr;
        }

        ++lineNr;
        linePtr = strtok_r(NULL, lineDelimiter, &lineSavePtr);
    }
    Serial.println("==============");

    return true;
}

bool Backend::toolsWithAccess(Config &config, MFRC522::Uid cardId, byte cardSecret[]) {
    char uidString[32];
    arrayToString(cardId.uidByte, 7, uidString);

    char cardSecretString[128];
    arrayToString(cardSecret, 32, cardSecretString);

    String url = config.backendUrl + deviceMac + "/permissions/" + String(uidString) + "/" + String(cardSecretString);

    Serial.print("Backend::toolsWithAccess Url=");
    Serial.println(url);

    hc.begin(url);

    String auth = base64::encode(deviceMac + ":" + secret);
    hc.addHeader("Authorization", "Basic " + auth);

    long before = millis();
    int httpCode = hc.GET();
    long diff = millis() - before;
    Serial.printf("Backend::toolsWithAccess delta t=%ld\n", diff);

    Serial.printf("Backend::toolsWithAccess httpCode=%i\n", httpCode);

    char accessBuffer[64];

    String accessFileName = String("/") + uidString + String(".txt");

    if (httpCode == 200) {
        String payload = hc.getString();
        
        Serial.printf("Backend::toolsWithAccess HTTP Code %i, Payload:\n", httpCode);
        Serial.println(payload);

        strncpy(accessBuffer, payload.c_str(), 64);
        accessBuffer[63] = '\0';

        Serial.printf("writing access file to SD card (%s)...\n", accessFileName.c_str());
        File accessFile = SD.open(accessFileName, "w");
        if (accessFile) {
            accessFile.printf(accessBuffer);
            accessFile.close();
            Serial.printf("done writing access file to SD card\n");
        } else {
            Serial.printf("could not open access file on SD card\n");
        }
    } else {
        String payload = hc.getString();
        Serial.printf("Backend::toolsWithAccess HTTP Code %i\nPayload: ", httpCode);
        Serial.println(payload);

        File accessFile = SD.open(accessFileName, "r");
        if (accessFile) {
            String payload = accessFile.readString();
            Serial.printf("Backend::readConfig from SD card\n");

            strncpy(accessBuffer, payload.c_str(), 63);
            accessBuffer[63] = '\0';

            Serial.printf("accessBuffer:\n%s", accessBuffer);

            accessFile.close();
        } else {
            Serial.printf("could not open /config.txt on SD card\n");
            return false;
        }
    }

    char lineDelimiter[] = "\n";
    char *linePtr;

    linePtr = strtok(accessBuffer, lineDelimiter);

    accessToolsAmount = 0;
    int accessToolNr = 0;
    while (linePtr != NULL) {
        accessTools[accessToolNr] = atoi(linePtr);
        ++accessToolsAmount;
        ++accessToolNr;

        linePtr = strtok(NULL, lineDelimiter);
    }
    return true;
}

bool Backend::downloadBgImage(Config &config) {
    File bgFile = SD.open("/bg.bmp", "w");
    if (bgFile) {
        Serial.println("Downloading Background image...");

        hc.begin(config.bgImageUrl);
        
        Serial.printf("Background image url: %s\n", config.bgImageUrl.c_str());

        int httpCode = hc.GET();
        if (httpCode == HTTP_CODE_OK) {
            hc.writeToStream(&bgFile);
            Serial.println();
            bgFile.close();
            return true;
        } else {
            Serial.printf("Did not recieve HTTP OK: %i\n", httpCode);
        }
        
        bgFile.close();
    } else {
        Serial.println("Could not open SD-Card /bg.bmp");
    }
    return false;
}

String Backend::split(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Backend::arrayToString(byte array[], unsigned int len, char buffer[]){
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}