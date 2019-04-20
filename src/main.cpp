#include "main.h"

const char* ssid = "FabLab Muenchen";
const char* password = "FabLab2016";

Backend backend;
CardReader cardreader;

bool accessEnabled = false;
unsigned long lastSuccessfulRead = 0;
byte cardId[10];

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

	loop_off();

	if (!loop_wifi()) {
		return;
	}

	if (loop_access()) {
		enable_access(0);
	} else {
		disable_access(0);
	}
}

bool loop_off() {
	//M5.Lcd.drawString("[centre]", 160, 230);
	M5.Lcd.drawString("[off]", 255, 230);
	if (M5.BtnC.wasReleased()) M5.powerOFF();
}

bool loop_wifi() {
	if (WiFi.status() == WL_CONNECTED) {
		return true;
	} else {
		Serial.println("WiFi not connected");
		return false;
	}
}

bool loop_access() {
	long deltaTime = millis() - lastSuccessfulRead;

	if (accessEnabled && deltaTime < 5000) {
		return true;
	}

	int uidSize = cardreader.read(false);

	if (uidSize <= 0) {
		lastSuccessfulRead = 0;
		return false;
	}

	char uidBuffer[12];
	if (uidSize == 4) {
		sprintf(uidBuffer, "%02X%02X%02X%02X", cardreader.uid.uidByte[0], cardreader.uid.uidByte[1], cardreader.uid.uidByte[2], cardreader.uid.uidByte[3]);
	} else {
		M5.Lcd.printf("UID length not 4!");
		return false;
	}


	//TODO: check whether uid is still the same

	//if card id is not the same, do new request

	String uidString(uidBuffer);
	bool access = backend.onlineRequest(DEVICE_ID, uidString);

	if (access) {
		M5.Lcd.printf("ACCESS");
		lastSuccessfulRead = millis();
		return true;
	} else {
		M5.Lcd.printf("NO ACCESS");
		return false;
	}
}

bool enable_access(int nr) {
	accessEnabled = true;
	M5.Lcd.fillCircle(160, 120, 60, TFT_GREEN);
}

bool disable_access(int nr) {
	accessEnabled = false;
	M5.Lcd.fillCircle(160, 120, 60, TFT_RED);
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