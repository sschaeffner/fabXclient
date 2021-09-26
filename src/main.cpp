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

#include "trace.h"
#include "result.h"
#include "conf.h"
#include "sdcard.h"
#include "misc.h"

namespace fabx{

static sdcard sdcard0(SD, TFCARD_CS_PIN, SPI, 40 * 1000 * 1000);
static Adafruit_MCP23008 gpio0, gpio1;
void setup()
{
    M5.begin();

    delay(50);

    Trace::set_uart(Serial);
    TRACE_INFO("Trace started!");

    SPI.begin();
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    if(esp_wifi_set_ps(WIFI_PS_NONE)==ESP_OK){
        TRACE_INFO("WiFi Powersaving disabled!");
    }
    else{
        TRACE_ERROR("Disable WiFi Powersaving failed");
    }
    
    sdcard0.begin();
    sdcard0.listdir("/", 5);
    TRACE_INFO("Initilize IO Expanders");
    Wire.begin();
    fabx::i2cscan(Serial, Wire);

    gpio0.begin(0);
    gpio0.writeGPIO(0xFF);
    gpio1.begin(1);
    gpio1.writeGPIO(0xFF);
    for(int i = 0; i<8; i++){
        gpio0.pinMode(i, OUTPUT);
        gpio1.pinMode(i, OUTPUT);   
    }
    TRACE_INFO("Initilize Backend connection");
}

void loop(){
   
}

} //namespace fabx