#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <esp32-hal-bt.c>
#include <esp_wifi.h>
#include <EEPROM.h>
#include <MFRC522.h>
#include <Adafruit_MCP23008.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

#include "conf.h"
#include "deviceconfig.h"
#include "cardreader.h"
#include "backend.h"

void setup_secret();
void loop_bg();
void loop_ntp();
bool loop_wifi();
void loop_config();
void loop_access();
void dump_byte_array(byte *buffer, byte bufferSize);
void lcd_dump_byte_array(byte *buffer, byte bufferSize);

int toolNrToToolIndex(int toolNr);

void playRecSound();
void playRecSoundT(void * param);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);

enum State { IDLE, CARD_ID_KNOWN, ACCESS_KNOWN, CHOOSE_TOOL, UNLOCK_TOOL, KEEP_CARD, KEEP_CARD_STILL, CHECK_CARD };

#endif //MAIN_H