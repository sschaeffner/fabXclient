#include "main.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;

wl_status_t wifiStatus = WL_IDLE_STATUS;
unsigned long lastWifiReconnect = 0;

bool ntpSynced = false;

bool redrawRequest, redrawing;

int configReadTry = 0;
bool configRead;

int bgImageReadTry = 0;
bool bgImageRead;

CardReader cardReader;
Config config;
Backend backend;

Adafruit_MCP23008 gpio0, gpio1;

State state;
MFRC522::Uid cardId;
int lastTimeCardRead = 0;;

int lastTimeToolSelectorChanged = 0;
int toolSelector;
int accessToolId;

bool countDownDisplayed = false;

AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;
TaskHandle_t taskRecSound;

void i2cscan(){
	Serial.println(" Scanning I2C Addresses");
	uint8_t cnt=0;
	for(uint8_t i=0;i<128;i++){
		Wire.beginTransmission(i);
		uint8_t ec = Wire.endTransmission(true);
		if(ec == 0){
			if(i<16)Serial.print('0');
			Serial.print(i,HEX);
			cnt++;
		}
		else Serial.print("..");
		Serial.print(' ');
		if ((i & 0x0f) == 0x0f)Serial.println();
	}
	Serial.print("Scan Completed, ");
	Serial.print(cnt);
	Serial.println(" I2C Devices found.");
}

void setup() {
	M5.begin();

	delay(50);

	Serial.begin(115200);
	Serial.println("Hello World");

	SPI.begin();

	WiFi.begin(ssid, password);
	esp_err_t wifi_success = esp_wifi_set_ps(WIFI_PS_NONE);
	if (wifi_success == ESP_OK) {
		Serial.println("WiFi set to no powersaving");
	} else {
		Serial.println("Error when trying to set WiFi to no powersaving");
	}

	if (SD.begin(TFCARD_CS_PIN, SPI, 40000000)) {
		Serial.println("SD Card begin successful");
	} else {
		Serial.println("SD Card begin NOT successful");
	}
	listDir(SD, "/", 5);

	//M5.Speaker.tone(440, 100);

	Wire.begin();
	i2cscan();
	Serial.println();

	Serial.println("GPIO Expander IODIR");
	gpio0.begin(0);
	gpio0.writeGPIO(0xFF);
	gpio0.pinMode(0, OUTPUT);
	gpio0.pinMode(1, OUTPUT);
	gpio0.pinMode(2, OUTPUT);
	gpio0.pinMode(3, OUTPUT);
	gpio0.pinMode(4, OUTPUT);
	gpio0.pinMode(5, OUTPUT);
	gpio0.pinMode(6, OUTPUT);
	gpio0.pinMode(7, OUTPUT);
	

	gpio1.begin(1);
	gpio1.writeGPIO(0xFF);
	gpio1.pinMode(0, OUTPUT);
	gpio1.pinMode(1, OUTPUT);
	gpio1.pinMode(2, OUTPUT);
	gpio1.pinMode(3, OUTPUT);
	gpio1.pinMode(4, OUTPUT);
	gpio1.pinMode(5, OUTPUT);
	gpio1.pinMode(6, OUTPUT);
	gpio1.pinMode(7, OUTPUT);
	

	Serial.println("GPIO Expander IODIR done");


	backend.begin();
	cardReader.begin();
	cardId.size = 0;

	setup_secret();

	lastWifiReconnect = millis();

	//disable Bluetooth
	btStop();

	configRead = false;
	bgImageRead = false;

	Serial.printf("deviceMac:%s\n", backend.deviceMac.c_str());

	redrawRequest = true;
	redrawing = false;

	M5.Lcd.fillCircle(80, 120, 20, TFT_LIGHTGREY);
	M5.Lcd.fillCircle(160, 120, 20, TFT_LIGHTGREY);
	M5.Lcd.fillCircle(240, 120, 20, TFT_LIGHTGREY);
}

void setup_secret() {
	EEPROM.begin(SECRET_LENGTH + 1);
	byte magic = EEPROM.readByte(0);

	if (magic != 0x42) {
		Serial.println("GENERATING NEW SECRET!");
		//generate new secret
		EEPROM.writeByte(0, 0x42);
		for (int i = 0; i < SECRET_LENGTH; i++) {
			EEPROM.writeByte(i + 1, (byte) esp_random());
		}
	} else {
		Serial.println("READING SECRET");
	}

	char buffer[3];

	for (int i = 0; i < SECRET_LENGTH; i++) {
		byte b = EEPROM.readByte(i + 1);
		sprintf(buffer, "%02x", b);
		backend.secret += buffer;
	}

	EEPROM.end();
}

void loop() {
	M5.update();

	redrawing = redrawRequest;
	redrawRequest = false;

	if (redrawing) M5.Lcd.clearDisplay();

	loop_bg();
	loop_wifi();
	loop_ntp();
	loop_config();
	loop_access();

	delay(1);
	yield();
}

void loop_bg() {
	if (state == IDLE) {
		if (redrawing) {
			M5.Lcd.drawBmpFile(SD, "/bg.bmp", 0, 0);
		}
	}
}

bool loop_wifi() {
	wl_status_t status = WiFi.status();
	if (status != wifiStatus) {
		redrawRequest = true;
		Serial.printf("wifi status change from %i to %i\n", wifiStatus, status);
		wifiStatus = status;
	}

	if (redrawing) {
		M5.Lcd.setTextColor(TFT_WHITE);
		M5.Lcd.setTextDatum(TR_DATUM);
		M5.Lcd.setTextSize(1);
		M5.Lcd.drawString(backend.deviceMac.c_str(), 320, 0);
	}

	if (wifiStatus == WL_CONNECTED) {
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

void loop_ntp() {
	if (wifiStatus == WL_CONNECTED && !ntpSynced) {
		Serial.println("attempting NTP sync...");
		struct tm local;
		configTzTime(TZ_INFO, NTP_SERVER);
		ntpSynced = getLocalTime(&local, 5000);
		if (ntpSynced) {
			Serial.println(&local, "NTP: Date: %d.%m.%y Time: %H:%M:%S");
		}
		redrawRequest = true;
	}
	if (redrawing && !ntpSynced) {
		M5.Lcd.setTextDatum(CC_DATUM);
		M5.Lcd.setTextColor(TFT_WHITE);
		M5.Lcd.setTextSize(3);
		M5.Lcd.drawString("NTP SYNC...", 160, 120);
	}
}

void loop_config() {
	if (wifiStatus == WL_CONNECTED && !configRead) {
		++configReadTry;
		Serial.printf("configReadTry=%i...\n", configReadTry);
		configRead = backend.readConfig(config, configReadTry >= CONFIG_TRIES_BEFORE_CACHE);
		if (configRead) {
			redrawRequest = true;
		}
		Serial.printf("configRead=%i\n", configRead);
	}
	if (wifiStatus == WL_CONNECTED && !bgImageRead && bgImageReadTry <= BG_IMAGE_MAX_TRIES) {
		++bgImageReadTry;
		Serial.printf("bgImageReadTry=%i...\n", bgImageReadTry);
		bgImageRead = backend.downloadBgImage(config);
		if (bgImageRead) {
			redrawRequest = true;
		}
		Serial.printf("bgImageRead=%i\n", bgImageRead);
	}
	if (redrawing) {
		if (configRead) {
			M5.Lcd.setTextColor(0xFC82);
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
		if (cardId.size == 4) {
			sprintf(cardIdBuffer, "0x%02X%02X%02X%02X", cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3]);
		} else if (cardId.size == 7) {
			sprintf(cardIdBuffer, "0x%02X%02X%02X%02X%02X%02X%02X", cardId.uidByte[0], cardId.uidByte[1], cardId.uidByte[2], cardId.uidByte[3], cardId.uidByte[4], cardId.uidByte[5], cardId.uidByte[6]);
		}
		

		M5.Lcd.setTextColor(TFT_WHITE);
		M5.Lcd.setTextDatum(TL_DATUM);
		M5.Lcd.setTextSize(1);
		M5.Lcd.drawString(cardIdBuffer, 0, 20);
	}

	if (state == IDLE) {
		cardId.size = 0;
		lastTimeCardRead = 0;
		lastTimeToolSelectorChanged = 0;

		cardReader.begin();
		delay(10);
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

		playRecSound();

		if (backend.toolsWithAccess(config, cardId, cardReader.cardSecret)) {
			state = ACCESS_KNOWN;
		} else {
			state = IDLE;
			redrawRequest = true;
		}
    }

	if (state == ACCESS_KNOWN) {
		if (backend.accessToolsAmount == 1) {
			accessToolId = backend.accessTools[0];

			int accessToolIndex = toolNrToToolIndex(accessToolId);
			String toolName = config.toolNames[accessToolIndex];

			Serial.printf("only one tool available, skipping tool selection\n");
			Serial.printf("accessToolId: %i\n", accessToolId);
			Serial.printf("accessToolIndex: %i\n", accessToolIndex);
			Serial.printf("accessToolName: %s\n", toolName.c_str());

			switch (config.toolModes[accessToolIndex]) {
				case KEEP:
					state = KEEP_CARD;
					break;
				case UNLOCK:
					state = UNLOCK_TOOL;
					break;
			}
			redrawRequest = true;
		} else if (backend.accessToolsAmount > 1) {
			state = CHOOSE_TOOL;
			toolSelector = 0;
			redrawRequest = true;
			lastTimeToolSelectorChanged = millis();
			Serial.printf("accessToolsAmount %i\n", backend.accessToolsAmount);
		} else {
			/*M5.Speaker.begin();
			M5.Speaker.tone(1047, 1);
			delay(150);
			M5.Speaker.tone(784, 1);
			delay(150);
			M5.Speaker.tone(659, 1);
			delay(150);
			M5.Speaker.tone(523, 1);
			delay(150);
			M5.Speaker.mute();
			M5.Speaker.end(); */

			Serial.printf("accessToolsAmount zero\n");
			state = IDLE;
			redrawRequest = true;
		}
	}

	if (state == CHOOSE_TOOL) {
		bool listRedraw = false;
		if (M5.BtnA.wasPressed()) {
			if (toolSelector > 0) --toolSelector;
			//redrawRequest = true;
			listRedraw = true;
			lastTimeToolSelectorChanged = millis();
		}
		if (M5.BtnB.wasPressed()) {
			if (toolSelector < backend.accessToolsAmount - 1) ++toolSelector;
			//redrawRequest = true;
			listRedraw = true;
			lastTimeToolSelectorChanged = millis();
		}
		if (M5.BtnC.wasPressed()) {
			accessToolId = backend.accessTools[toolSelector];
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

		if (millis() > lastTimeToolSelectorChanged + 10000) {
			state = IDLE;
			redrawRequest = true;
		}

		if (redrawing || listRedraw) {
			M5.Lcd.setTextDatum(BC_DATUM);
			M5.Lcd.setTextColor(TFT_WHITE);
			M5.Lcd.setTextSize(1);
			M5.Lcd.drawString("[up]", 65, 240);
			M5.Lcd.drawString("[down]", 160, 240);
			M5.Lcd.drawString("[select]", 255, 240);
		
			M5.Lcd.setTextSize(3);
			int fontHeight = M5.Lcd.fontHeight(M5.Lcd.textfont);
			fontHeight = fontHeight + (fontHeight >> 1);// fontHeight *= 1.5

			int nrActualTools = 0;

			for (int i = 0; i < backend.accessToolsAmount; i++) {
				int toolNr = backend.accessTools[i];
				int toolIndex = toolNrToToolIndex(toolNr);

				if (toolIndex >= 0) {
					if (i == toolSelector) {
						M5.Lcd.setTextColor(TFT_GREEN);
					} else {
						M5.Lcd.setTextColor(TFT_WHITE);
					}

					if (nrActualTools < 5) {
						M5.Lcd.setTextDatum(TL_DATUM);
						M5.Lcd.drawString(config.toolNames[toolIndex], 0, 36 + (fontHeight * i));
					} else {
						M5.Lcd.setTextDatum(TR_DATUM);
						M5.Lcd.drawString(config.toolNames[toolIndex], 320, 36 + (fontHeight * (i - 5)));
					}
					
					nrActualTools++;
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

		gpio1.digitalWrite(accessToolPin - 1, LOW);

		M5.Lcd.clearDisplay();
		M5.Lcd.setTextDatum(CC_DATUM);
		M5.Lcd.setTextColor(TFT_GREEN);
		M5.Lcd.setTextSize(3);
		M5.Lcd.drawString(config.toolNames[accessToolIndex], 160, 120);

		delay(200);

		gpio1.writeGPIO(0xFF);
		
		state = IDLE;
		redrawRequest = true;
	}

	if (state == KEEP_CARD) {
		Serial.printf("KEEP TOOL: accessToolId=%i\n", accessToolId);

		int accessToolIndex = toolNrToToolIndex(accessToolId);
		int accessToolPin = config.toolPins[accessToolIndex];

		Serial.printf("KEEP TOOL: accessToolIndex=%i\n", accessToolIndex);
		
		Serial.print("KEEP TOOL: toolName=");
		Serial.println(config.toolNames[accessToolIndex]);

		Serial.printf("KEEP TOOL: pin=%i\n", accessToolPin);

		//TURN ON TOOL
		gpio1.digitalWrite(accessToolPin - 1, LOW);

		state = KEEP_CARD_STILL;
	}

	if (state == KEEP_CARD_STILL) {
		int accessToolIndex = toolNrToToolIndex(accessToolId);

		if (redrawing) {
			M5.Lcd.setTextDatum(CC_DATUM);
			M5.Lcd.setTextColor(TFT_GREEN);
			M5.Lcd.setTextSize(3);
			M5.Lcd.drawString(config.toolNames[accessToolIndex], 160, 120);
		}

		if (millis() > lastTimeCardRead + 3000) {
			Serial.printf("KEEP CARD PRECHECK\n");

			cardReader.begin();
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

				if (countDownDisplayed) {
					redrawRequest = true;
					countDownDisplayed = false;
				}
			} else { // card not found
				M5.Lcd.clearDisplay(TFT_RED);

				int secsLeft = (config.toolTimes[accessToolIndex] + 3000) / 1000 - (millis() - lastTimeCardRead) / 1000;

				char countDown[5];
				sprintf(countDown, "%i", secsLeft);

				M5.Lcd.setTextDatum(CC_DATUM);
				M5.Lcd.setTextColor(TFT_BLACK);
				M5.Lcd.setTextSize(7);
				M5.Lcd.drawString(countDown, 160, 120);

				countDownDisplayed = true;
			}
		}
		if (millis() > lastTimeCardRead + config.toolTimes[accessToolIndex] + 3000) {
			state = CHECK_CARD;
		}
	}

	if (state == CHECK_CARD) {
		Serial.printf("CHECK_CARD\n");

		cardReader.begin();
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
			
			state = KEEP_CARD_STILL;
		} else { // card no longer there or card id changed
			cardId.size = 0;
			lastTimeCardRead = 0;

			gpio1.writeGPIO(0xFF);

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

void playRecSound() {
	xTaskCreate(
      playRecSoundT, /* Function to implement the task */
      "playRecSoundT", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      100,  /* Priority of the task */
      &taskRecSound);  /* Task handle. */
}

void playRecSoundT(void * param) {
	//C5 523.25
	//E5 659.25
	//G5 783.99
	//C6 1046.50
	M5.Speaker.begin();
	M5.Speaker.setVolume(1);
	M5.Speaker.tone(523, 1);
	delay(150);
	M5.Speaker.tone(659, 1);
	delay(150);
	M5.Speaker.tone(784, 1);
	delay(150);
	M5.Speaker.tone(1047, 1);
	delay(150);
	M5.Speaker.mute();
	M5.Speaker.end();

	vTaskDelete(NULL);
}

void playRecSoundT2(void * param) {
	file = new AudioFileSourceSD("/recSound.wav");
	out = new AudioOutputI2S(0, 1); // Output to builtInDAC
	out->SetOutputModeMono(true);
	wav = new AudioGeneratorWAV();
	wav->begin(file, out);

	while (wav->isRunning()) {
		if (!wav->loop()) wav->stop();
	}
	Serial.printf("WAV done\n");
	wav->stop();
	out->stop();
	file->close();

	vTaskDelete(NULL);
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

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}