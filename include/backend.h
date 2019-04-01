#ifndef BACKEND_H
#define BACKEND_H

#include <Arduino.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <MFRC522.h>
#include "deviceconfig.h"

class Backend {
    public:
        String deviceMac;
        String secret;
        int accessTools[MAX_TOOLS];
        int accessToolsAmount;

        void begin();
        bool readConfig(Config &config, bool allowCached);
        bool downloadBgImage(Config &config);
        bool toolsWithAccess(Config &config, MFRC522::Uid cardId, byte cardSecret[]);
        
    private:
        String split(String data, char separator, int index);
        void arrayToString(byte array[], unsigned int len, char buffer[]);
};

#endif //BACKEND_H