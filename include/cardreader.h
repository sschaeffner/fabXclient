#ifndef CARDREADER_H
#define CARDREADER_H

#define SS_PIN 2
#define RST_PIN 5

#include <Arduino.h>
#include <MFRC522.h>
// #include <M5Stack.h>

#include "conf.h"

class CardReader {
    public:
        CardReader() : mfrc522(SS_PIN, RST_PIN) {};
        MFRC522::Uid uid;
        byte cardSecret[32];

        int begin();
        int read(bool lcdDebug);

    private:
        MFRC522 mfrc522;
        MFRC522::StatusCode status; 
        MFRC522::MIFARE_Key key;

        byte secretKey[16] = PICC_PSK;

        boolean initCardReader();
        boolean wakeupAndSelect();
        boolean authWithSecretKey();
        boolean readCardSecret();
        void endCard();

        void error(String msg);
        void info(String msg);
        void infoByteArray(byte *buffer, byte bufferSize);
        void debug(String msg);
        void debugByteArray(byte *buffer, byte bufferSize);
};

#endif //CARDREADER_H