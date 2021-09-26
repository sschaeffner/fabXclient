#pragma once
#include <Arduino.h>
#include "trace.h"
#include <Wire.h>

namespace fabx{

void i2cscan(HardwareSerial& iSerial, TwoWire& iWire){
	TRACE_INFO(" Scanning I2C Addresses");
	uint8_t cnt=0;
	for(uint8_t i=0;i<128;i++){
		iWire.beginTransmission(i);
		uint8_t ec = iWire.endTransmission(true);
		if(ec == 0){
			if(i<16)TRACE_INFO("0");
			TRACE_INFO("%x",i);
			cnt++;
		}
		else TRACE_INFO("..");
		TRACE_INFO(" ");
		if ((i & 0x0f) == 0x0f)TRACE_INFO("\n\r");
	}
	TRACE_INFO("Scan Completed, ");
	TRACE_INFO("%d",cnt);
	TRACE_INFO(" I2C Devices found.");
}
}