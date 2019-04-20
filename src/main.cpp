#include "main.h"

const char* ssid = "FabLab Muenchen";
const char* password = "FabLab2016";

Backend backend;
CardReader cardreader;

bool successfulRead;

void setup() {
	Serial.begin(115200);

	WiFi.begin(ssid, password);

	SPI.begin();

	M5.begin();
	M5.Lcd.fillScreen(WHITE);
	delay(50);
	M5.Lcd.clearDisplay();

	cardreader.begin();

	Serial.println("\n\nHello World");

}

void loop() {
	M5.update();
	if (M5.BtnC.wasReleased()) M5.powerOFF();

	if (!loop_wifi()) {
		delay(10);
		return;
	}

	if (successfulRead && !M5.BtnA.wasReleased()) {
		return;
	}

	successfulRead = false;

	M5.Lcd.clearDisplay();
	M5.Lcd.setCursor(0, 0);

	//M5.Lcd.drawString("[centre]", 160, 230);
	M5.Lcd.drawString("[off]", 255, 230);

	int uidSize = cardreader.read(true);

	if (uidSize > 0) {
		M5.Lcd.setTextDatum(CC_DATUM);
		M5.Lcd.drawString("[read]", 65, 230);

		successfulRead = true;
	}

	char uidBuffer[12];
	if (uidSize == 4) {
		sprintf(uidBuffer, "%02X%02X%02X%02X", cardreader.uid.uidByte[0], cardreader.uid.uidByte[1], cardreader.uid.uidByte[2], cardreader.uid.uidByte[3]);
	} else {
		M5.Lcd.printf("UID length not 4!");
		return;
	}

	String uidString(uidBuffer);
	bool access = backend.onlineRequest(DEVICE_ID, uidString);

	if (access) {
		M5.Lcd.printf("ACCESS");
	} else {
		M5.Lcd.printf("NO ACCESS");
	}
	delay(5000);
}

bool loop_wifi() {
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("WiFi connected");
		return true;
	} else {
		Serial.println("WiFi not connected");
		return false;
	}
}

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void lcd_dump_byte_array(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
        M5.Lcd.print(buffer[i] < 0x10 ? " 0" : " ");
        M5.Lcd.print(buffer[i], HEX);
    }
}