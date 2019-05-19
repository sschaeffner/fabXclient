#ifndef MAIN_H
#define MAIN_H

#define WIFI_RECONNECT_TIME 5000 // how long the ESP should wait until it disables and reenables WiFi if it cannot connect

#include <Arduino.h>
#include <SPI.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <esp32-hal-bt.c>
#include <MFRC522.h>

#include "config.h"
#include "cardreader.h"
#include "backend.h"

void loop_off();
bool loop_wifi();
void loop_config();
void loop_access();
void dump_byte_array(byte *buffer, byte bufferSize);
void lcd_dump_byte_array(byte *buffer, byte bufferSize);

int toolNrToToolIndex(int toolNr);

enum State { IDLE, CARD_ID_KNOWN, ACCESS_KNOWN, CHOOSE_TOOL, UNLOCK_TOOL, KEEP_CARD, KEEP_CARD_STILL, CHECK_CARD };

#endif //MAIN_H