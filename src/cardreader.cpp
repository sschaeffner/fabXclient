#include "cardreader.h"

int CardReader::begin() {
	key.keyByte[0] = 0xFF;
	key.keyByte[1] = 0xFF;
	key.keyByte[2] = 0xFF;
	key.keyByte[3] = 0xFF;
	key.keyByte[4] = 0xFF;
	key.keyByte[5] = 0xFF;

	pinMode(RST_PIN, OUTPUT);
	digitalWrite(RST_PIN, LOW);
	delayMicroseconds(2); // 8.8.1 Reset timing requirements says about 100ns. Let us be generous: 2μsl
	digitalWrite(RST_PIN, HIGH);

    mfrc522.PCD_Init(SS_PIN, RST_PIN);

	byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF)) {
		Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
		return false;
	}
	
	return true;
}

boolean CardReader::initCardReader() {
	// hard reset PCD
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);
    delayMicroseconds(20); // 8.8.1 Reset timing requirements says about 100ns. Let us be generous: 2μs
    digitalWrite(RST_PIN, HIGH);

    // init PCD
    mfrc522.PCD_Init(SS_PIN, RST_PIN);

    // check PCD software version (0x92 is normal)
    byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    // When 0x00 or 0xFF is returned, communication probably failed
    if ((v == 0x00) || (v == 0xFF)) {
        debug(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
        return false;
    } else {
        //info("MFRC522 version: 0x" + String(v, 16));
    }

    // leave antenna gain at default (0x40), more makes communication less reliable
    //debug("Antenna Gain: 0x" + String(mfrc522.PCD_GetAntennaGain(), 16));

    // Reset baud rates
    mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
    mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
    // Reset ModWidthReg
    mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);

    // random delay (helps quite a lot!)
    delay(5);

	//reset uid buffer
    uid.size = 0;
    uid.uidByte[0] = 0;
    uid.uidByte[1] = 0;
    uid.uidByte[2] = 0;
    uid.uidByte[3] = 0;
    uid.uidByte[4] = 0;
    uid.uidByte[5] = 0;
    uid.uidByte[6] = 0;
    uid.uidByte[7] = 0;
    uid.uidByte[8] = 0;
    uid.uidByte[9] = 0;
    uid.sak = 0;

	return true;
}

boolean CardReader::wakeupAndSelect() {
	byte waBufferATQA[2];
    byte waBufferSize = 2;

    waBufferATQA[0] = 0x00;
    waBufferATQA[1] = 0x00;

    status = mfrc522.PICC_WakeupA(waBufferATQA, &waBufferSize);
    
    //debug("CardReader::read WakeupA status = ");
    //debug(MFRC522::GetStatusCodeName(status));

    if (status != MFRC522::STATUS_OK) {
        endCard();
        return false;
    }

    info("ATQA: ");
    infoByteArray(waBufferATQA, waBufferSize);

	if (waBufferATQA[0] != 0x44 || waBufferATQA[1] != 0x00) {
		error("Only Ultralight C supported!");
		endCard();
		return false;
	}

    status = mfrc522.PICC_Select(&uid, 0);
    debug("Select status = ");
    debug(MFRC522::GetStatusCodeName(status));

    if (status != MFRC522::STATUS_OK) {
        endCard();
        return 1;
    }

    info("Uid: ");
    infoByteArray(uid.uidByte, uid.size);

	return true;
}

boolean CardReader::authWithSecretKey() {
    status = mfrc522.MIFARE_UL_C_Auth(secretKey);
    debug("auth with secret key = ");
    debug(MFRC522::GetStatusCodeName(status));
    if (status != MFRC522::STATUS_OK) {
        endCard();
        return false;
    }
	info("authentication with secret key success");
	return true;
}

boolean CardReader::readCardSecret() {
	memset(cardSecret, 0, 32);

	byte readBuffer[32];
	byte readBufferSize = 32;

	for (int p = 0x20; p <= 0x27; p += 4) {
		memset(readBuffer, 0, 32);
        readBufferSize = 32; // after read: how many bytes actually were read
        status = mfrc522.MIFARE_Read(p, readBuffer, &readBufferSize);

		if (status == MFRC522::STATUS_OK) {
			debug("page " + String(p) + " readBuffer: ");
			debugByteArray(readBuffer, readBufferSize);

            if (readBufferSize == 18) {
                memcpy(&cardSecret[(p - 0x20) * 4], readBuffer, 16);
            } else {
                error("expected answer length 18, got " + String(readBufferSize));
            }
		} else {
            debug("Read status = ");
            debug(MFRC522::GetStatusCodeName(status));
            endCard();
            return false;
        }
	}

	return true;
}

int CardReader::read(bool lcdDebug) {
	if(!initCardReader()) {
		error("Could not initialize RC522!");
		return -1;
	}

	if (!wakeupAndSelect()) {
		//error("Could not select card!");
		return -2;
	}

	if (!authWithSecretKey()) {
		error("Could not authenticate with secret key!");
		return -3;
	}

	if (!readCardSecret()) {
		error("Could not read cardSecret!");
		return -4;
	}

	return 1;
}

void CardReader::endCard() {
    mfrc522.PCD_StopCrypto1();
    mfrc522.PICC_HaltA();
}

void CardReader::error(String msg) {
	Serial.print("[E] ");
	Serial.println(msg);
}

void CardReader::info(String msg) {
	Serial.print("[I] ");
	Serial.println(msg);
}

void CardReader::infoByteArray(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void CardReader::debug(String msg) {
	Serial.print("[D] ");
	Serial.println(msg);
}

void CardReader::debugByteArray(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println();
}