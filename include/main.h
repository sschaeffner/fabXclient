#ifndef MAIN_H
#define MAIN_H

#define DEVICE_ID 1

#include <Arduino.h>
#include <SPI.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <MFRC522.h>

#include "cardreader.h"
#include "backend.h"

bool loop_off();
bool loop_wifi();
bool loop_access();
bool enable_access(int nr);
bool disable_access(int nr);
void dump_byte_array(byte *buffer, byte bufferSize);
void lcd_dump_byte_array(byte *buffer, byte bufferSize);

#endif //MAIN_H