#include "backend.h"
#include <HTTPClient.h>


bool Backend::onlineRequest(int deviceId, String cardId) {
    HTTPClient hc;
    char urlBuffer[60];
    sprintf(urlBuffer, "http://srmbp.fablab:8081/backend/%u/access/%s", deviceId, cardId.c_str());
    
    Serial.print("Backend::onlineRequest Url=");
    Serial.println(urlBuffer);

    hc.begin(urlBuffer);

    int httpCode = hc.GET();

    if (httpCode == 200) {
        String payload = hc.getString();

        int accessIndex = payload.indexOf("ACCESS");
        int cardIdIndex = payload.indexOf(cardId);

        Serial.printf("Backend::onlineRequest accessIndex=%i, cardIdIndex=%i\n", accessIndex, cardIdIndex);

        if (accessIndex >= 0 && cardIdIndex >= 0) {
            Serial.println("");
            return true;
        } else {
            return false;
        }
    } else if (httpCode >= 400 && httpCode < 600) {
        String payload = hc.getString();
        Serial.printf("Backend::onlineRequest HTTP Code %i\nPayload: ", httpCode);
        Serial.println(payload);
        return false;
    }
}