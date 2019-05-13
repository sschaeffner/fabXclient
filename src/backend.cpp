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

bool Backend::readConfig(Config &config) {
    HTTPClient hc;
    char urlBuffer[128];
    sprintf(urlBuffer, "https://fabcess.moehritz.de/backend/config/%s", deviceMac.c_str());

    Serial.print("Backend::readConfig Url=");
    Serial.println(urlBuffer);

    hc.begin(urlBuffer);

    int httpCode = hc.GET();

    Serial.printf("Backend::readConfig httpCode=%i\n", httpCode);

    if (httpCode == 200) {
        String payload = hc.getString();
        char configBuffer[512];
        strncpy(configBuffer, payload.c_str(), 512);
        configBuffer[511] = '\0';

        Serial.printf("Backend::readConfig HTTP Code %i\n", httpCode);
        Serial.printf("configBuffer:\n%s", configBuffer);

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
    } else {
        String payload = hc.getString();
        Serial.printf("Backend::readConfig HTTP Code %i, Payload:\n", httpCode);
        Serial.println(payload);
        return false;
    }
}

bool Backend::toolsWithAccess(MFRC522::Uid cardId) {
    HTTPClient hc;
    char urlBuffer[128];
    sprintf(urlBuffer, "https://fabcess.moehritz.de/backend/access/%s/0x%02X%02X%02X%02X", deviceMac.c_str(), cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3]);

    Serial.print("Backend::toolsWithAccess Url=");
    Serial.println(urlBuffer);

    hc.begin(urlBuffer);

    int httpCode = hc.GET();

    Serial.printf("Backend::toolsWithAccess httpCode=%i\n", httpCode);

    if (httpCode == 200) {
        String payload = hc.getString();
        
        Serial.printf("Backend::toolsWithAccess HTTP Code %i, Payload:\n", httpCode);
        Serial.println(payload);

        char accessBuffer[64];
        strncpy(accessBuffer, payload.c_str(), 64);
        accessBuffer[63] = '\0';

        char lineDelimiter[] = "\n";
        char *linePtr;

        linePtr = strtok(accessBuffer, lineDelimiter);

        accessToolsAmount = 1;
        int accessToolNr = 0;
        while (linePtr != NULL) {
            accessTools[accessToolNr] = atoi(linePtr);
            ++accessToolsAmount;
            ++accessToolNr;

            linePtr = strtok(NULL, lineDelimiter);
        }

        return true;
    } else {
        String payload = hc.getString();
        Serial.printf("Backend::toolsWithAccess HTTP Code %i\nPayload: ", httpCode);
        Serial.println(payload);
        return false;
    }
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