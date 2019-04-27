#include "cardreader.h"

void CardReader::begin() {
    mfrc522.PCD_Init(SS_PIN, RST_PIN);
	mfrc522.PCD_DumpVersionToSerial();

	key.keyByte[0] = 0xFF;
	key.keyByte[1] = 0xFF;
	key.keyByte[2] = 0xFF;
	key.keyByte[3] = 0xFF;
	key.keyByte[4] = 0xFF;
	key.keyByte[5] = 0xFF;
}

int CardReader::read(bool lcdDebug) {
    // Reset baud rates
	mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
	mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
	// Reset ModWidthReg
	mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);

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

	byte waBufferATQA[2];
	byte waBufferSize = 2;

	waBufferATQA[0] = 0x00;
	waBufferATQA[1] = 0x00;

	status = mfrc522.PICC_WakeupA(waBufferATQA, &waBufferSize);
	
	Serial.print("CardReader::read WakeupA status = ");
	Serial.println(MFRC522::GetStatusCodeName(status));
    if (lcdDebug) {
        M5.Lcd.print("CardReader::read WakeupA status = ");
        M5.Lcd.println(MFRC522::GetStatusCodeName(status));
    }

	if (status != MFRC522::STATUS_OK) {
		end_read();
		return -1;
	}

	Serial.print("ATQA: ");
	dump_byte_array(waBufferATQA, waBufferSize);
	Serial.println();
    if (lcdDebug) {
        M5.Lcd.print("ATQA: ");
        lcd_dump_byte_array(waBufferATQA, waBufferSize);
        M5.Lcd.println();
    }

	status = mfrc522.PICC_Select(&uid, 0);
	Serial.print("Select status = ");
	Serial.println(MFRC522::GetStatusCodeName(status));
    if (lcdDebug) {
        M5.Lcd.print("Select status = ");
        M5.Lcd.println(MFRC522::GetStatusCodeName(status));
    }

	if (status != MFRC522::STATUS_OK) {
		end_read();
		return -2;
	}

	Serial.print("Uid: ");
	dump_byte_array(uid.uidByte, uid.size);
	Serial.println();
    if (lcdDebug) {
        M5.Lcd.print("Uid: ");
        lcd_dump_byte_array(uid.uidByte, uid.size);
        M5.Lcd.println();
    }

	byte rBuffer[18];
	byte rBufferSize = 18;
	memset(rBuffer, 0, rBufferSize);

	for (int sector = 0; sector < 4; sector++) {
		Serial.printf("Sector %u\n", sector);
        if (lcdDebug) M5.Lcd.printf("Sector %u\n", sector);

		status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector * 4, &key, &uid);
		Serial.print("Authenticate status = ");
		Serial.println(MFRC522::GetStatusCodeName(status));
		if (status != mfrc522.STATUS_OK) {
            if (lcdDebug) {
                M5.Lcd.print("Authenticate status = ");
                M5.Lcd.println(MFRC522::GetStatusCodeName(status));
            }

			end_read();
            return -3;
		}

		for (int block = 0; block < 4; block++) {
			Serial.printf("Block %u (%u)\n", block, sector * 4 + block);

			status = mfrc522.MIFARE_Read(sector * 4 + block, rBuffer, &rBufferSize);
			Serial.print("Read status = ");
			Serial.println(MFRC522::GetStatusCodeName(status));
			if (status != mfrc522.STATUS_OK) {
                if (lcdDebug) {
                    M5.Lcd.print("Read status = ");
                    M5.Lcd.println(MFRC522::GetStatusCodeName(status));
                }

				end_read();
                return -4;
			}
			
			Serial.printf("Block %u content: ", sector * 4 + block);
			dump_byte_array(rBuffer, rBufferSize);
			Serial.println();

            if (lcdDebug) {
                lcd_dump_byte_array(rBuffer, 16);
                M5.Lcd.println();
            }
		}
	}

	Serial.print("\n\n\n");
    end_read();
    return uid.size;
}

void CardReader::end_read() {
	mfrc522.PCD_StopCrypto1();
	mfrc522.PICC_HaltA();
}

void CardReader::dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void CardReader::lcd_dump_byte_array(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
        M5.Lcd.print(buffer[i] < 0x10 ? " 0" : " ");
        M5.Lcd.print(buffer[i], HEX);
    }
}