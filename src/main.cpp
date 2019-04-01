#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include "main.h"

#define SS_PIN 22
#define RST_PIN 21

MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::StatusCode status; 
MFRC522::Uid myUid;
MFRC522::MIFARE_Key key;

void setup() {
	Serial.begin(115200);
	SPI.begin();

	Serial.println("\n\nHello World");

	mfrc522.PCD_Init(SS_PIN, RST_PIN);
	mfrc522.PCD_DumpVersionToSerial();

	key.keyByte[0] = 0xFF;
	key.keyByte[1] = 0xFF;
	key.keyByte[2] = 0xFF;
	key.keyByte[3] = 0xFF;
	key.keyByte[4] = 0xFF;
	key.keyByte[5] = 0xFF;

	/*key.keyByte[0] = 0x00;
	key.keyByte[1] = 0x00;
	key.keyByte[2] = 0x00;
	key.keyByte[3] = 0x00;
	key.keyByte[4] = 0x00;
	key.keyByte[5] = 0x00;*/
}

void loop() {
	// Reset baud rates
	mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
	mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
	// Reset ModWidthReg
	mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);

	//reset uid buffer
	myUid.size = 0;
	myUid.uidByte[0] = 0;
	myUid.uidByte[1] = 0;
	myUid.uidByte[2] = 0;
	myUid.uidByte[3] = 0;
	myUid.uidByte[4] = 0;
	myUid.uidByte[5] = 0;
	myUid.uidByte[6] = 0;
	myUid.uidByte[7] = 0;
	myUid.uidByte[8] = 0;
	myUid.uidByte[9] = 0;
	myUid.sak = 0;

	byte waBufferATQA[2];
	byte waBufferSize = 2;

	waBufferATQA[0] = 0x00;
	waBufferATQA[1] = 0x00;

	status = mfrc522.PICC_WakeupA(waBufferATQA, &waBufferSize);
	Serial.print("WakeupA status = ");
	Serial.println(MFRC522::GetStatusCodeName(status));

	if (status != MFRC522::STATUS_OK) {
		delay(1000);
		return;
	}

	Serial.print("ATQA: ");
	dump_byte_array(waBufferATQA, waBufferSize);
	Serial.println();

	status = mfrc522.PICC_Select(&myUid, 0);
	Serial.print("Select status = ");
	Serial.println(MFRC522::GetStatusCodeName(status));

	if (status != MFRC522::STATUS_OK) {
		delay(1000);
		return;
	}

	Serial.print("Uid: ");
	dump_byte_array(myUid.uidByte, myUid.size);
	Serial.println();

	byte rBuffer[18];
	byte rBufferSize = 18;
	memset(rBuffer, 0, rBufferSize);

	for (int sector = 0; sector < 2; sector++) {
		Serial.printf("Sector %u\n", sector);

		status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector * 4, &key, &myUid);
		Serial.print("Authenticate status = ");
		Serial.println(MFRC522::GetStatusCodeName(status));

		for (int block = 0; block < 4; block++) {
			Serial.printf("Block %u (%u)\n", block, sector * 4 + block);

			status = mfrc522.MIFARE_Read(sector * 4 + block, rBuffer, &rBufferSize);
			Serial.print("Read status = ");
			Serial.println(MFRC522::GetStatusCodeName(status));

			Serial.printf("Block %u content: ", sector * 4 + block);
			dump_byte_array(rBuffer, rBufferSize);
			Serial.println();
		}
	}

	mfrc522.PCD_StopCrypto1();
	mfrc522.PICC_HaltA();

	Serial.print("\n\n\n");

	delay(1000);
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