#include "backend.h"
#include <HTTPClient.h>

void Backend::begin() {
    // read device mac into String
    byte mac[6];
    WiFi.macAddress(mac);
    char macBuffer[13];
    sprintf(macBuffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    deviceMac = String(macBuffer);
}

bool Backend::readConfig(Config &config, bool allowCached) {
    HTTPClient hc;
    char urlBuffer[128];
    sprintf(urlBuffer, "https://fabx.cloud/backend/config/%s/%s", deviceMac.c_str(), secret.c_str());

    Serial.print("Backend::readConfig Url=");
    Serial.println(urlBuffer);

    hc.begin(urlBuffer);

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
            Serial.printf("Device Name: %s\n", linePtr);
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

bool Backend::toolsWithAccess(MFRC522::Uid cardId) {
    HTTPClient hc;
    char urlBuffer[128];
    sprintf(urlBuffer, "https://fabx.cloud/backend/access/%s/0x%02X%02X%02X%02X", deviceMac.c_str(), cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3]);

    Serial.print("Backend::toolsWithAccess Url=");
    Serial.println(urlBuffer);

    hc.begin(urlBuffer);

    int httpCode = hc.GET();

    Serial.printf("Backend::toolsWithAccess httpCode=%i\n", httpCode);

    char accessBuffer[64];

    char accessFileName[64];
    sprintf(accessFileName, "/0x%02X%02X%02X%02X.txt", cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3]);

    if (httpCode == 200) {
        String payload = hc.getString();
        
        Serial.printf("Backend::toolsWithAccess HTTP Code %i, Payload:\n", httpCode);
        Serial.println(payload);

        strncpy(accessBuffer, payload.c_str(), 64);
        accessBuffer[63] = '\0';

        Serial.printf("writing access file to SD card...\n");
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