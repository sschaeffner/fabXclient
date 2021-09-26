#pragma once
#include <SD.h>

namespace fabx{

class sdcard
{

public:
    sdcard(fs::SDFS& iSDFS, uint8_t iSSPin, SPIClass& iSPI, uint32_t iFrequency);
    void set_filesystem(fs::SDFS& iSDFS);
    void set_serial(HardwareSerial& iSerial);
    bool begin();
    void listdir(const char* dirname, uint8_t levels);

private:
    fs::SDFS* mSDFS;
    uint8_t mCSPin;
    SPIClass* mSPI;
    uint32_t mFreq;


};

inline sdcard::sdcard(fs::SDFS& iSDFS, uint8_t iSSPin, SPIClass& iSPI, uint32_t iFrequency)
    :mSDFS(&iSDFS)
    ,mCSPin(iSSPin)
    ,mSPI(&iSPI)
    ,mFreq(iFrequency)
{
    
}
inline void sdcard::set_filesystem(fs::SDFS& iSDFS){
    mSDFS = &iSDFS;
}

inline bool sdcard::begin(){
    bool ret = false;
    ret = mSDFS->begin(mCSPin, *mSPI, mFreq);
    if (ret) TRACE_INFO("SD Card initialized.");
    else TRACE_ERROR("SD not initialized");
    return ret;
}
inline void sdcard::listdir(const char* dirname, uint8_t levels){
   TRACE_INFO("Listing directory: %s\n", dirname);

    File root = mSDFS->open(dirname);
    if(!root){
        TRACE_INFO("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        TRACE_INFO("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            TRACE_INFO("  DIR : ");
            TRACE_INFO(file.name());
            if(levels){
                sdcard::listdir(file.name(), levels -1);
            }
        } else {
            TRACE_INFO("  FILE: ");
            TRACE_INFO(file.name());
            TRACE_INFO("  SIZE: ");
            TRACE_INFO(String(file.size()).c_str());
        }
        file = root.openNextFile();
    }

}
}