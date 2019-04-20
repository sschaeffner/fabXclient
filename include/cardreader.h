#ifndef CARDREADER_H
#define CARDREADER_H

#define SS_PIN 22
#define RST_PIN 21

#include <Arduino.h>
#include <MFRC522.h>
#include <M5Stack.h>

class CardReader {
    public:
        CardReader() : mfrc522(SS_PIN, RST_PIN) {};
        MFRC522::Uid uid;

        void begin();
        int read(bool lcdDebug);

    private:
        MFRC522 mfrc522;
        MFRC522::StatusCode status; 
        MFRC522::MIFARE_Key key;
        void dump_byte_array(byte *buffer, byte bufferSize);
        void lcd_dump_byte_array(byte *buffer, byte bufferSize);
};

#endif //CARDREADER_H