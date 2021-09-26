#pragma once
#include <stdint.h>
#include <Arduino.h>

namespace fabx{
    

#ifdef NO_TRACE
    #define TRACE_ERROR(...)
    #define TRACE_DEBUG(...)
    #define TRACE_INFO(...)
#else
    #define TRACE_ERROR(iFormat, ...) fabx::Trace::print(fabx::DEBUG_ERROR, iFormat, ##__VA_ARGS__)
    #define TRACE_DEBUG(iFormat, ...) fabx::Trace::print(fabx::DEBUG_DEBUG, iFormat, ##__VA_ARGS__)
    #define TRACE_INFO(iFormat, ...) fabx::Trace::print(fabx::DEBUG_INFO, iFormat, ##__VA_ARGS__)
#endif

enum {
    DEBUG_ERROR = 0,
    DEBUG_DEBUG = 1,
    DEBUG_INFO = 2,
};

class Trace
{
public:
    static void print(uint32_t iLevel, const char* iFormat, ...);
    static void set_uart(HardwareSerial& iSerial);
private:
    static HardwareSerial* mUart;
};

inline void Trace::set_uart(HardwareSerial& iSerial)
{
    mUart = &iSerial;
}

inline void Trace::print(uint32_t iLevel, const char* iFormat, ...)
{
    va_list va;
    va_start(va, iFormat);
    mUart->printf(iFormat, va);
}

}