#include "main.h"

const char* ssid = "FabLab Muenchen";
const char* password = "FabLab2016";

wl_status_t wifiStatus = WL_IDLE_STATUS;
unsigned long lastWifiReconnect = 0;

bool redrawRequest, redrawing;

bool configRead;

CardReader cardReader;
Config config;
Backend backend;

State state;
MFRC522::Uid cardId;
int lastTimeCardRead = 0;;

int toolSelector;
int accessToolId;

void setup() {
	Serial.begin(115200);
	WiFi.begin(ssid, password);
	SPI.begin();
	M5.begin();
	//M5.Speaker.tone(440, 100);

	backend.begin();
	cardReader.begin();
	cardId.size = 0;

	lastWifiReconnect = millis();

	//disable Bluetooth
	btStop();

	configRead = false;
	
	Serial.println("\n\nHello World");

	Serial.printf("deviceMac:%s\n", backend.deviceMac.c_str());

	redrawRequest = true;
	redrawing = false;

	M5.Lcd.fillCircle(80, 120, 20, TFT_LIGHTGREY);
	M5.Lcd.fillCircle(160, 120, 20, TFT_LIGHTGREY);
	M5.Lcd.fillCircle(240, 120, 20, TFT_LIGHTGREY);
}

void loop() {
	M5.update();

	redrawing = redrawRequest;
	redrawRequest = false;

	if (redrawing) M5.Lcd.clearDisplay();

	loop_off();
	loop_wifi();
	loop_config();
	loop_access();

	delay(1);
	yield();
}

void loop_off() {
	if (state == IDLE) {
		if (redrawing) {
			M5.Lcd.setTextColor(TFT_WHITE);
			M5.Lcd.setTextDatum(BC_DATUM);
			M5.Lcd.setTextSize(1);
			M5.Lcd.drawString("[off]", 65, 240);
		}

		if (M5.BtnA.wasReleased()) M5.powerOFF();
	}
}

bool loop_wifi() {
	wl_status_t status = WiFi.status();
	if (status != wifiStatus) {
		redrawRequest = true;
		wifiStatus = status;
	}

	if (redrawing) {
		M5.Lcd.setTextColor(TFT_WHITE);
		M5.Lcd.setTextDatum(TR_DATUM);
		M5.Lcd.setTextSize(1);
		M5.Lcd.drawString(backend.deviceMac.c_str(), 320, 0);
	}

	if (WiFi.status() == WL_CONNECTED) {
		if (redrawing) {
			M5.Lcd.setTextColor(TFT_GREEN);
			M5.Lcd.setTextDatum(TR_DATUM);
			M5.Lcd.setTextSize(1);
			M5.Lcd.drawString("WiFi CONN", 320, 12);
		}
		
		return true;
	} else {
		if (redrawing) {
			M5.Lcd.setTextColor(TFT_RED);
			M5.Lcd.setTextDatum(TR_DATUM);
			M5.Lcd.setTextSize(1);
			M5.Lcd.drawString("WiFi DISC", 320, 12);
		}

		Serial.println("WiFi not connected.");

		if (millis() - lastWifiReconnect > WIFI_RECONNECT_TIME) {
			Serial.println("WiFi: Trying to reconnect...");

			WiFi.disconnect(true);
			WiFi.mode(WIFI_OFF);
			WiFi.mode(WIFI_STA);
			WiFi.begin(ssid, password);

			lastWifiReconnect = millis();
		} else {
			Serial.println("WiFi: Last reconnect try within the last 5 seconds.");
		}

		return WiFi.status() == WL_CONNECTED;
	}
}

void loop_config() {
	if (wifiStatus == WL_CONNECTED && !configRead) {
		configRead = backend.readConfig(config);
		redrawRequest = true;
	}
	if (redrawing) {
		if (configRead) {
			M5.Lcd.setTextColor(TFT_WHITE);
			M5.Lcd.setTextDatum(TL_DATUM);
			M5.Lcd.setTextSize(2);
			M5.Lcd.drawString(config.deviceName, 0, 0);
		} else {
			M5.Lcd.setTextDatum(CC_DATUM);
			M5.Lcd.setTextColor(TFT_WHITE);
			M5.Lcd.setTextSize(3);
			M5.Lcd.drawString("SELF CONFIG", 160, 120);
		}
	}
}

void loop_access() {
	if (state >= CARD_ID_KNOWN && redrawing) {
		char cardIdBuffer[32];
		sprintf(cardIdBuffer, "0x%02X%02X%02X%02X", cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3]);

		M5.Lcd.setTextColor(TFT_WHITE);
		M5.Lcd.setTextDatum(TL_DATUM);
		M5.Lcd.setTextSize(1);
		M5.Lcd.drawString(cardIdBuffer, 0, 20);
	}

	if (state == IDLE) {
		cardId.size = 0;
		lastTimeCardRead = 0;

        int success = cardReader.read(false);

        if (success > 0) {
			cardId.size = cardReader.uid.size;
			lastTimeCardRead = millis();

			for (int i = 0; i < cardId.size; i++) {
				cardId.uidByte[i] = cardReader.uid.uidByte[i];
			}

            state = CARD_ID_KNOWN;
        }
    }

    if (state == CARD_ID_KNOWN) {
		M5.Lcd.fillCircle(80, 120, 20, TFT_WHITE);
		M5.Lcd.fillCircle(160, 120, 20, TFT_WHITE);
		M5.Lcd.fillCircle(240, 120, 20, TFT_WHITE);
		if (backend.toolsWithAccess(cardId)) {
			state = ACCESS_KNOWN;
		}
    }

	if (state == ACCESS_KNOWN) {
		if (config.toolAmount == 1 && backend.accessToolsAmount == 1 && config.toolIds[0] == backend.accessTools[0]) {
			accessToolId = config.toolIds[0];

			switch (config.toolModes[0]) {
				case KEEP:
					state = KEEP_CARD;
					break;
				case UNLOCK:
					state = UNLOCK_TOOL;
					break;
			}
		} else {
			if (backend.accessToolsAmount > 0) {
				state = CHOOSE_TOOL;
				toolSelector = 0;
				redrawRequest = true;
				Serial.printf("accessToolsAmount %i\n", backend.accessToolsAmount);
			} else {
				Serial.printf("accessToolsAmount zero\n");
				state = IDLE;
				redrawRequest = true;
			}
		}
	}

	if (state == CHOOSE_TOOL) {
		if (M5.BtnA.wasPressed()) {
			if (toolSelector > 0) --toolSelector;
			redrawRequest = true;
		}
		if (M5.BtnB.wasPressed()) {
			if (toolSelector < backend.accessToolsAmount - 1) ++toolSelector;
			redrawRequest = true;
		}
		if (M5.BtnC.wasPressed()) {
			accessToolId = backend.accessTools[toolSelector];
			//accessToolId = config.toolIds[toolSelector];
			int accessToolIndex = toolNrToToolIndex(accessToolId);

			switch (config.toolModes[accessToolIndex]) {
				case KEEP:
					state = KEEP_CARD;
					break;
				case UNLOCK:
					state = UNLOCK_TOOL;
					break;
			}
			redrawRequest = true;
		}

		if (redrawing) {
			M5.Lcd.setTextDatum(BC_DATUM);
			M5.Lcd.setTextColor(TFT_WHITE);
			M5.Lcd.setTextSize(1);
			M5.Lcd.drawString("[up]", 65, 240);
			M5.Lcd.drawString("[down]", 160, 240);
			M5.Lcd.drawString("[select]", 255, 240);
		
			M5.Lcd.setTextDatum(TL_DATUM);
			M5.Lcd.setTextSize(2);
			int fontHeight = M5.Lcd.fontHeight(M5.Lcd.textfont);
			fontHeight = fontHeight + (fontHeight >> 1);// fontHeight *= 1.5

			for (int i = 0; i < backend.accessToolsAmount; i++) {
				int toolNr = backend.accessTools[i];
				int toolIndex = toolNrToToolIndex(toolNr);

				if (toolIndex >= 0) {
					if (i == toolSelector) {
						M5.Lcd.setTextColor(TFT_GREEN);
					} else {
						M5.Lcd.setTextColor(TFT_WHITE);
					}
					
					M5.Lcd.drawString(config.toolNames[toolIndex], 0, 36 + (fontHeight * i));
				}
			}
		}
	}

	if (state == UNLOCK_TOOL) {
		Serial.printf("UNLOCK TOOL: accessToolId=%i\n", accessToolId);

		int accessToolIndex = toolNrToToolIndex(accessToolId);
		int accessToolPin = config.toolPins[accessToolIndex];

		Serial.printf("UNLOCK TOOL: accessToolIndex=%i\n", accessToolIndex);

		Serial.print("UNLOCK TOOL: toolName=");
		Serial.println(config.toolNames[accessToolIndex]);

		Serial.printf("UNLOCK TOOL: pin=%i\n", accessToolPin);

		M5.Lcd.clearDisplay();
		M5.Lcd.setTextDatum(CC_DATUM);
		M5.Lcd.setTextColor(TFT_GREEN);
		M5.Lcd.setTextSize(3);
		M5.Lcd.drawString(config.toolNames[accessToolIndex], 160, 120);

		delay(200);
		
		state = IDLE;
	}

	if (state == KEEP_CARD) {
		Serial.printf("KEEP TOOL: accessToolId=%i\n", accessToolId);

		int accessToolIndex = toolNrToToolIndex(accessToolId);
		int accessToolPin = config.toolPins[accessToolIndex];

		Serial.printf("KEEP TOOL: accessToolIndex=%i\n", accessToolIndex);
		
		Serial.print("KEEP TOOL: toolName=");
		Serial.println(config.toolNames[accessToolIndex]);

		Serial.printf("KEEP TOOL: pin=%i\n", accessToolPin);

		if (redrawing) {
			M5.Lcd.setTextDatum(CC_DATUM);
			M5.Lcd.setTextColor(TFT_GREEN);
			M5.Lcd.setTextSize(3);
			M5.Lcd.drawString(config.toolNames[accessToolIndex], 160, 120);
		}

		//TODO: ENABLE TOOL

		state = KEEP_CARD_STILL;
	}

	if (state == KEEP_CARD_STILL) {
		if (millis() > lastTimeCardRead + 5000) {
			state = CHECK_CARD;
		}
	}

	if (state == CHECK_CARD) {
		Serial.printf("CHECK_CARD\n");

        int success = cardReader.read(false);

        if (success > 0) {
			// check if read card id and access card id are the same (no card hot-swap)
			for (int i = 0; i < cardId.size; i++) {
				if (cardId.uidByte[i] != cardReader.uid.uidByte[i]) {
					success = 0;
				}
			}
		}

		if (success > 0) { // card id is still the same
			cardId.size = cardReader.uid.size;
			lastTimeCardRead = millis();
			
			state = KEEP_CARD;
		} else { // card no longer there or card id changed
			cardId.size = 0;
			lastTimeCardRead = 0;
			
			//TODO: DISABLE TOOL
			state = IDLE;
		}
		redrawRequest = true;
	}
}

int toolNrToToolIndex(int toolNr) {
	for (int i = 0; i < config.toolAmount; i++) {
		if (config.toolIds[i] == toolNr) {
			return i;
		}
	}
	return -1;
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