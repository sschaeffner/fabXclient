#ifndef MAIN_H
#define MAIN_H

#define WIFI_RECONNECT_TIME 5000 // how long the ESP should wait until it disables and reenables WiFi if it cannot connect
#define SECRET_LENGTH 16
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time

#include <Arduino.h>
#include <SPI.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <esp32-hal-bt.c>
#include <EEPROM.h>
#include <MFRC522.h>

#include "config.h"
#include "cardreader.h"
#include "backend.h"

void setup_secret();
void loop_off();
void loop_ntp();
bool loop_wifi();
void loop_config();
void loop_access();
void dump_byte_array(byte *buffer, byte bufferSize);
void lcd_dump_byte_array(byte *buffer, byte bufferSize);

int toolNrToToolIndex(int toolNr);

enum State { IDLE, CARD_ID_KNOWN, ACCESS_KNOWN, CHOOSE_TOOL, UNLOCK_TOOL, KEEP_CARD, KEEP_CARD_STILL, CHECK_CARD };

#endif //MAIN_H