#ifndef BACKEND_H
#define BACKEND_H

#include <Arduino.h>
#include <WiFi.h>
#include <MFRC522.h>
#include "config.h"

class Backend {
    public:
        String deviceMac;
        String secret;
        int accessTools[MAX_TOOLS];
        int accessToolsAmount;

        void begin();
        bool readConfig(Config &config);
        bool toolsWithAccess(MFRC522::Uid cardId);
        
    private:
        String split(String data, char separator, int index);
};

#endif //BACKEND_H