#ifndef BACKEND_H
#define BACKEND_H

#include <Arduino.h>
#include <SD.h>
// #include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include "deviceconfig.h"

class Backend {
    public:
        String deviceMac;
        String secret;
        int accessTools[MAX_TOOLS];
        int accessToolsAmount;

        void begin();
        void loop();
        bool readConfig(Config &config, bool allowCached);
        bool downloadBgImage(Config &config);
        bool toolsWithAccess(Config &config, MFRC522::Uid cardId, byte cardSecret[]);
        
    private:
        WebSocketsClient webSocket;

        String split(String data, char separator, int index);
        void arrayToString(byte array[], unsigned int len, char buffer[]);

        static void websocketEvent(WStype_t type, uint8_t * payload, size_t length);
};

#endif //BACKEND_H