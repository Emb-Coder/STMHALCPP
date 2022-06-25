/*
 * Pn532.cpp
 *
 *  Created on: 17 juin 2022
 *      Author: larbi
 *
 *  inspired by Adafruit library of the pn532 module :
 *   https://github.com/adafruit/Adafruit-PN532/blob/master/Adafruit_PN532.cpp
 *
 */

#include <Pn532.h>

uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5};

#define PN532_PACKBUFFSIZ 64

uint8_t pn532_packetbuffer[PN532_PACKBUFFSIZ];

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif


Pn532::Pn532(SpiInstancesTypeDef spiInstance,
		std::pair<GpioPortsTypeDef, uint16_t> ssPin)
{
	_spiInstance = spiInstance;
	_ssPin = ssPin;
}

Pn532::Pn532(TwiInstancesTypeDef twiInstance,
		std::pair<GpioPortsTypeDef, uint16_t> irqPin,
		std::pair<GpioPortsTypeDef, uint16_t> rstPin)
{
	_twiInstance = twiInstance;
	_irqPin = irqPin;
	_rstPin = rstPin;
	_useTwi = true;
}

DriversExecStatus Pn532::begin(void)
{
	if (_useTwi)
	{
		pn532Twi = Twi::getInstance(_twiInstance);

	    Twi::twi_settings_t settings = {
	        .clockSpeed= TwiSpeedModeTypeDef::STANDARD_MODE,
	        .slaveAddLength= 7,
	        .slaveAddress = 0
	    };

	    if (STATUS_OK != pn532Twi->begin(settings))
	    {
	    	return (STATUS_ERROR);
	    }

	    pn532Irq = new Gpio(_irqPin.first, _irqPin.second, GpioModeTypeDef::INPUT);

	    if (STATUS_OK != pn532Irq->begin())
			return (STATUS_ERROR);

	    pn532Rst = new Gpio(_rstPin.first, _rstPin.second, GpioModeTypeDef::OUTPUT);

	    if (STATUS_OK != pn532Rst->begin())
			return (STATUS_ERROR);

	    // Reset the PN532
	    pn532Rst->digitalWrite(HIGH);
	    pn532Rst->digitalWrite(LOW);

	    HAL_Delay(400);

	    pn532Rst->digitalWrite(HIGH);

	    HAL_Delay(10); // Small delay required before taking other actions after reset.
	    // See timing diagram on page 209 of the datasheet, section 12.23.

	}
	else
	{
		pn532Spi = Spi::getInstance(_spiInstance);

		Spi::spi_settings_t settings = {
			.dataSize = SpiDataSizeTypeDef::EIGHT_BITS,
			.firstBit = SpiFirstBitTypeDef::MSB,
			.clockPrescaler = SpiClockPrescalerTypeDef::PSC_64,
			.clockMode = SpiClockModeTypeDef::MODE_0
		};

		pn532Cs = new Gpio(_ssPin.first, _ssPin.second, GpioModeTypeDef::OUTPUT);

		if (STATUS_OK != pn532Cs->begin())
	    	return (STATUS_ERROR);

	    if (STATUS_OK != pn532Spi->begin(settings, pn532Cs))
	    {
	    	return (STATUS_ERROR);
	    }

	    // not exactly sure why but we have to send a dummy command to get synced up
	    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
	    sendCommandCheckAck(pn532_packetbuffer, 1);
	    // ignore response!
	}

	return (STATUS_OK);
}

bool Pn532::SAMConfig(void)
{
	pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
	pn532_packetbuffer[1] = 0x01; // normal mode;
	pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
	pn532_packetbuffer[3] = 0x01; // use IRQ pin!

	if (!sendCommandCheckAck(pn532_packetbuffer, 4))
		return false;

	// read data packet
	readdata(pn532_packetbuffer, 8);

	int offset = 6;
	return (pn532_packetbuffer[offset] == 0x15);
}

uint32_t Pn532::getFirmwareVersion(void)
{
	uint32_t response;

	pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

	if (!sendCommandCheckAck(pn532_packetbuffer, 1)) {
		return 0;
	}

	// read data packet
	readdata(pn532_packetbuffer, 12);

	// check some basic stuff
	if (0 != memcmp((char *)pn532_packetbuffer,
			(char *)pn532response_firmwarevers, 6)) {

		if (_debugger != nullptr)
			_debugger->println("Firmware doesn't match!");

		return 0;
	}

	int offset = 7;
	response = pn532_packetbuffer[offset++];
	response <<= 8;
	response |= pn532_packetbuffer[offset++];
	response <<= 8;
	response |= pn532_packetbuffer[offset++];
	response <<= 8;
	response |= pn532_packetbuffer[offset++];

	return response;
}

bool Pn532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen,
		uint16_t timeout)
{
	// uint16_t timer = 0;

	// write the command
	writecommand(cmd, cmdlen);

	// Wait for chip to say its ready!
	if (!waitready(timeout)) {
		return false;
	}

	if (_debugger != nullptr)
	{
		if (pn532Spi == NULL) {
			_debugger->println("IRQ received");
		}
	}

	// read acknowledgement
	if (!readack()) {
		if (_debugger != nullptr)
			_debugger->println("No ACK frame received!");

		return false;
	}

	// For SPI only wait for the chip to be ready again.
	// This is unnecessary with I2C.
	if (pn532Spi != NULL) {
		if (!waitready(timeout)) {
			return false;
		}
	}

	return true; // ack'd command
}

bool Pn532::writeGPIO(uint8_t pinstate)
{
	// uint8_t errorbit;

	// Make sure pinstate does not try to toggle P32 or P34
	pinstate |= (1 << PN532_GPIO_P32) | (1 << PN532_GPIO_P34);

	// Fill command buffer
	pn532_packetbuffer[0] = PN532_COMMAND_WRITEGPIO;
	pn532_packetbuffer[1] = PN532_GPIO_VALIDATIONBIT | pinstate; // P3 Pins
	pn532_packetbuffer[2] = 0x00; // P7 GPIO Pins (not used ... taken by SPI)

	if (_debugger != nullptr)
	{
		_debugger->print("Writing P3 GPIO: ");
		_debugger->println(pn532_packetbuffer[1], HEX);
	}

	// Send the WRITEGPIO command (0x0E)
	if (!sendCommandCheckAck(pn532_packetbuffer, 3))
		return 0x0;

	// Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0F) DATACHECKSUM
	// 00)
	readdata(pn532_packetbuffer, 8);

	if (_debugger != nullptr)
	{
		_debugger->print("Received: ");

		PrintHex(pn532_packetbuffer, 8);

		_debugger->println();
	}

	int offset = 6;
	return (pn532_packetbuffer[offset] == 0x0F);
}

uint8_t Pn532::readGPIO(void)
{
	pn532_packetbuffer[0] = PN532_COMMAND_READGPIO;

	// Send the READGPIO command (0x0C)
	if (!sendCommandCheckAck(pn532_packetbuffer, 1))
		return 0x0;

	// Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0D) P3 P7 IO1
	// DATACHECKSUM 00)
	readdata(pn532_packetbuffer, 11);

	/* READGPIO response should be in the following format:
	    byte            Description
	    -------------   ------------------------------------------
	    b0..5           Frame header and preamble (with I2C there is an extra 0x00)
	    b6              P3 GPIO Pins
	    b7              P7 GPIO Pins (not used ... taken by SPI)
	    b8              Interface Mode Pins (not used ... bus select pins)
	    b9..10          checksum */

	int p3offset = 7;



	if (_debugger != nullptr)
	{
		_debugger->print("Received: ");
		PrintHex(pn532_packetbuffer, 11);
		_debugger->println();
		_debugger->print("P3 GPIO: 0x");
		_debugger->println(pn532_packetbuffer[p3offset], HEX);
		_debugger->print("P7 GPIO: 0x");
		_debugger->println(pn532_packetbuffer[p3offset + 1], HEX);
		_debugger->print("IO GPIO: 0x");
		_debugger->println(pn532_packetbuffer[p3offset + 2], HEX);
		// Note: You can use the IO GPIO value to detect the serial bus being used
		switch (pn532_packetbuffer[p3offset + 2]) {
		case 0x00: // Using UART
			_debugger->println("Using UART (IO = 0x00)");
			break;
		case 0x01: // Using I2C
			_debugger->println("Using I2C (IO = 0x01)");
			break;
		case 0x02: // Using SPI
			_debugger->println("Using SPI (IO = 0x02)");
			break;
		}
	}


	return pn532_packetbuffer[p3offset];
}

bool Pn532::setPassiveActivationRetries(uint8_t maxRetries)
{
	pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
	pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
	pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
	pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
	pn532_packetbuffer[4] = maxRetries;


	if (_debugger != nullptr)
	{
		_debugger->print("Setting MxRtyPassiveActivation to ");
		_debugger->print(maxRetries, DEC);
		_debugger->println(" ");
	}

	if (!sendCommandCheckAck(pn532_packetbuffer, 5))
		return 0x0; // no ACK

	return 1;
}

bool Pn532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid,
		uint8_t *uidLength, uint16_t timeout)
{
	pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
	pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
	pn532_packetbuffer[2] = cardbaudrate;

	if (!sendCommandCheckAck(pn532_packetbuffer, 3, timeout)) {

		if (_debugger != nullptr)
		{
			_debugger->println("No card(s) read");
		}

		return 0x0; // no cards read
	}

	// wait for a card to enter the field (only possible with I2C)
	if (pn532Spi == NULL) {

		if (_debugger != nullptr)
		{
			_debugger->println("Waiting for IRQ (indicates card presence)");
		}
		if (!waitready(timeout)) {

			if (_debugger != nullptr)
			{
				_debugger->println("IRQ Timeout");
			}

			return 0x0;
		}
	}

	return readDetectedPassiveTargetID(uid, uidLength);
}

bool Pn532::startPassiveTargetIDDetection(uint8_t cardbaudrate)
{
	pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
	pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
	pn532_packetbuffer[2] = cardbaudrate;

	return sendCommandCheckAck(pn532_packetbuffer, 3);
}

bool Pn532::readDetectedPassiveTargetID(uint8_t *uid, uint8_t *uidLength)
{
	// read data packet
	readdata(pn532_packetbuffer, 20);
	// check some basic stuff

	/* ISO14443A card response should be in the following format:
	    byte            Description
	    -------------   ------------------------------------------
	    b0..6           Frame header and preamble
	    b7              Tags Found
	    b8              Tag Number (only one used in this example)
	    b9..10          SENS_RES
	    b11             SEL_RES
	    b12             NFCID Length
	    b13..NFCIDLen   NFCID                                      */


	if (_debugger != nullptr)
	{
		_debugger->print("Found ");
		_debugger->print(pn532_packetbuffer[7], DEC);
		_debugger->println(" tags");
	}

	if (pn532_packetbuffer[7] != 1)
		return 0;

	uint16_t sens_res = pn532_packetbuffer[9];
	sens_res <<= 8;
	sens_res |= pn532_packetbuffer[10];

	if (_debugger != nullptr)
	{
		_debugger->print("ATQA: 0x");
		_debugger->println(sens_res, HEX);
		_debugger->print("SAK: 0x");
		_debugger->println(pn532_packetbuffer[11], HEX);
	}

	/* Card appears to be Mifare Classic */
	*uidLength = pn532_packetbuffer[12];

	if (_debugger != nullptr)
	{
		_debugger->print("UID:");
	}

	for (uint8_t i = 0; i < pn532_packetbuffer[12]; i++) {
		uid[i] = pn532_packetbuffer[13 + i];

		if (_debugger != nullptr)
		{
			_debugger->print(" 0x");
			_debugger->print(uid[i], HEX);
		}
	}

	if (_debugger != nullptr)
	{
		_debugger->println();
	}

	return 1;
}

bool Pn532::inDataExchange(uint8_t *send, uint8_t sendLength, uint8_t *response,
		uint8_t *responseLength)
{
	if (sendLength > PN532_PACKBUFFSIZ - 2) {

		if (_debugger != nullptr)
		{
			_debugger->println("APDU length too long for packet buffer");
		}
		return false;
	}
	uint8_t i;

	pn532_packetbuffer[0] = 0x40; // PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = _inListedTag;
	for (i = 0; i < sendLength; ++i) {
		pn532_packetbuffer[i + 2] = send[i];
	}

	if (!sendCommandCheckAck(pn532_packetbuffer, sendLength + 2, 1000)) {

		if (_debugger != nullptr)
		{
			_debugger->println("Could not send APDU");
		}
		return false;
	}

	if (!waitready(1000)) {

		if (_debugger != nullptr)
		{
			_debugger->println("Response never received for APDU...");
		}
		return false;
	}

	readdata(pn532_packetbuffer, sizeof(pn532_packetbuffer));

	if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 &&
			pn532_packetbuffer[2] == 0xff) {
		uint8_t length = pn532_packetbuffer[3];
		if (pn532_packetbuffer[4] != (uint8_t)(~length + 1)) {

			if (_debugger != nullptr)
			{
				_debugger->println("Length check invalid");
				_debugger->println(length, HEX);
				_debugger->println((~length) + 1, HEX);
			}
			return false;
		}
		if (pn532_packetbuffer[5] == PN532_PN532TOHOST &&
				pn532_packetbuffer[6] == PN532_RESPONSE_INDATAEXCHANGE) {
			if ((pn532_packetbuffer[7] & 0x3f) != 0) {

				if (_debugger != nullptr)
				{
					_debugger->println("Status code indicates an error");
				}
				return false;
			}

			length -= 3;

			if (length > *responseLength) {
				length = *responseLength; // silent truncation...
			}

			for (i = 0; i < length; ++i) {
				response[i] = pn532_packetbuffer[8 + i];
			}
			*responseLength = length;

			return true;
		} else {
			if (_debugger != nullptr)
			{
				_debugger->print("Don't know how to handle this command: ");
				_debugger->println(pn532_packetbuffer[6], HEX);
			}
			return false;
		}
	} else {
		if (_debugger != nullptr)
		{
			_debugger->println("Preamble missing");
		}
		return false;
	}
}

bool Pn532::inListPassiveTarget()
{
	pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
	pn532_packetbuffer[1] = 1;
	pn532_packetbuffer[2] = 0;

	if (_debugger != nullptr)
	{
		_debugger->print("About to inList passive target");
	}

	if (!sendCommandCheckAck(pn532_packetbuffer, 3, 1000)) {

		if (_debugger != nullptr)
		{
			_debugger->println("Could not send inlist message");
		}
		return false;
	}

	if (!waitready(30000)) {
		return false;
	}

	readdata(pn532_packetbuffer, sizeof(pn532_packetbuffer));

	if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 &&
			pn532_packetbuffer[2] == 0xff) {
		uint8_t length = pn532_packetbuffer[3];
		if (pn532_packetbuffer[4] != (uint8_t)(~length + 1)) {

			if (_debugger != nullptr)
			{
				_debugger->println("Length check invalid");
				_debugger->println(length, HEX);
				_debugger->println((~length) + 1, HEX);
			}
			return false;
		}
		if (pn532_packetbuffer[5] == PN532_PN532TOHOST &&
				pn532_packetbuffer[6] == PN532_RESPONSE_INLISTPASSIVETARGET) {
			if (pn532_packetbuffer[7] != 1) {

				if (_debugger != nullptr)
				{
					_debugger->println("Unhandled number of targets inlisted");

					_debugger->println("Number of tags inlisted:");
					_debugger->println(pn532_packetbuffer[7]);
				}
				return false;
			}

			_inListedTag = pn532_packetbuffer[8];

			if (_debugger != nullptr)
			{
				_debugger->print("Tag number: ");
				_debugger->println(_inListedTag);
			}
			return true;
		} else {

			if (_debugger != nullptr)
			{
				_debugger->print("Unexpected response to inlist passive host");
			}
			return false;
		}
	} else {

		if (_debugger != nullptr)
		{
			_debugger->println("Preamble missing");
		}
		return false;
	}

	return true;
}

uint8_t Pn532::AsTarget()
{
	pn532_packetbuffer[0] = 0x8C;
	uint8_t target[] = {
			0x8C,             // INIT AS TARGET
			0x00,             // MODE -> BITFIELD
			0x08, 0x00,       // SENS_RES - MIFARE PARAMS
			0xdc, 0x44, 0x20, // NFCID1T
			0x60,             // SEL_RES
			0x01, 0xfe, // NFCID2T MUST START WITH 01fe - FELICA PARAMS - POL_RES
			0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xc0,
			0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, // PAD
			0xff, 0xff,                               // SYSTEM CODE
			0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44,
			0x33, 0x22, 0x11, 0x01, 0x00, // NFCID3t MAX 47 BYTES ATR_RES
			0x0d, 0x52, 0x46, 0x49, 0x44, 0x49, 0x4f,
			0x74, 0x20, 0x50, 0x4e, 0x35, 0x33, 0x32 // HISTORICAL BYTES
	};
	if (!sendCommandCheckAck(target, sizeof(target)))
		return false;

	// read data packet
	readdata(pn532_packetbuffer, 8);

	int offset = 6;
	return (pn532_packetbuffer[offset] == 0x15);
}

uint8_t Pn532::getDataTarget(uint8_t *cmd, uint8_t *cmdlen)
{
	uint8_t length;
	pn532_packetbuffer[0] = 0x86;
	if (!sendCommandCheckAck(pn532_packetbuffer, 1, 1000)) {
		if (_debugger != nullptr)
			_debugger->println("Error en ack");

		return false;
	}

	// read data packet
	readdata(pn532_packetbuffer, 64);
	length = pn532_packetbuffer[3] - 3;

	// if (length > *responseLength) {// Bug, should avoid it in the reading
	// target data
	//  length = *responseLength; // silent truncation...
	//}

	for (int i = 0; i < length; ++i) {
		cmd[i] = pn532_packetbuffer[8 + i];
	}
	*cmdlen = length;
	return true;
}

uint8_t Pn532::setDataTarget(uint8_t *cmd, uint8_t cmdlen)
{
	uint8_t length;
	// cmd1[0] = 0x8E; Must!

	if (!sendCommandCheckAck(cmd, cmdlen))
		return false;

	// read data packet
	readdata(pn532_packetbuffer, 8);
	length = pn532_packetbuffer[3] - 3;
	for (int i = 0; i < length; ++i) {
		cmd[i] = pn532_packetbuffer[8 + i];
	}
	// cmdl = 0
	cmdlen = length;

	int offset = 6;
	return (pn532_packetbuffer[offset] == 0x15);
}

bool Pn532::mifareclassic_IsFirstBlock(uint32_t uiBlock)
{
	// Test if we are in the small or big sectors
	if (uiBlock < 128)
		return ((uiBlock) % 4 == 0);
	else
		return ((uiBlock) % 16 == 0);
}

bool Pn532::mifareclassic_IsTrailerBlock(uint32_t uiBlock)
{
	// Test if we are in the small or big sectors
	if (uiBlock < 128)
		return ((uiBlock + 1) % 4 == 0);
	else
		return ((uiBlock + 1) % 16 == 0);
}

uint8_t Pn532::mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen,
		uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData)
{
	// uint8_t len;
	uint8_t i;

	// Hang on to the key and uid data
	memcpy(_key, keyData, 6);
	memcpy(_uid, uid, uidLen);
	_uidLen = uidLen;


	if (_debugger != nullptr)
	{
		_debugger->print("Trying to authenticate card ");
		PrintHex((const uint8_t*) _uid, _uidLen);
		_debugger->print("Using authentication KEY ");
		_debugger->print(keyNumber ? 'B' : 'A');
		_debugger->print(": ");
		PrintHex((const uint8_t*)_key, 6);
	}

	// Prepare the authentication command //
	pn532_packetbuffer[0] =
			PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
	pn532_packetbuffer[1] = 1;        /* Max card numbers */
	pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
	pn532_packetbuffer[3] =
			blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
	memcpy(pn532_packetbuffer + 4, _key, 6);
	for (i = 0; i < _uidLen; i++) {
		pn532_packetbuffer[10 + i] = _uid[i]; /* 4 byte card ID */
	}

	if (!sendCommandCheckAck(pn532_packetbuffer, 10 + _uidLen))
		return 0;

	// Read the response packet
	readdata(pn532_packetbuffer, 12);

	// check if the response is valid and we are authenticated???
	// for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
	// Mifare auth error is technically byte 7: 0x14 but anything other and 0x00
	// is not good
	if (pn532_packetbuffer[7] != 0x00) {

		if (_debugger != nullptr)
		{
			_debugger->print("Authentification failed: ");
			PrintHexChar(pn532_packetbuffer, 12);
		}
		return 0;
	}

	return 1;
}

uint8_t Pn532::mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data)
{

	if (_debugger != nullptr)
	{
		_debugger->print("Trying to read 16 bytes from block ");
		_debugger->println(blockNumber);
	}

	/* Prepare the command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1;               /* Card number */
	pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
	pn532_packetbuffer[3] =
			blockNumber; /* Block Number (0..63 for 1K, 0..255 for 4K) */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {

		if (_debugger != nullptr)
		{
			_debugger->println("Failed to receive ACK for read command");
		}
		return 0;
	}

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	/* If byte 8 isn't 0x00 we probably have an error */
	if (pn532_packetbuffer[7] != 0x00) {

		if (_debugger != nullptr)
		{
			_debugger->println("Unexpected response");
			PrintHexChar(pn532_packetbuffer, 26);
		}
		return 0;
	}

	/* Copy the 16 data bytes to the output buffer        */
	/* Block content starts at byte 9 of a valid response */
	memcpy(data, pn532_packetbuffer + 8, 16);

	/* Display data for debug if requested */

	if (_debugger != nullptr)
	{
		_debugger->print("Block ");
		_debugger->println(blockNumber);
		PrintHexChar(data, 16);
	}

	return 1;
}

uint8_t Pn532::mifareclassic_WriteDataBlock(uint8_t blockNumber,
		uint8_t *data)
{

	if (_debugger != nullptr)
	{
		_debugger->print("Trying to write 16 bytes to block ");
		_debugger->println(blockNumber);
	}

	/* Prepare the first command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1;                /* Card number */
	pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
	pn532_packetbuffer[3] =
			blockNumber; /* Block Number (0..63 for 1K, 0..255 for 4K) */
	memcpy(pn532_packetbuffer + 4, data, 16); /* Data Payload */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 20)) {

		if (_debugger != nullptr)
		{
			_debugger->println("Failed to receive ACK for write command");
		}
		return 0;
	}
	HAL_Delay(10);

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	return 1;
}

uint8_t Pn532::mifareclassic_FormatNDEF(void)
{
	uint8_t sectorbuffer1[16] = {0x14, 0x01, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1,
			0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
	uint8_t sectorbuffer2[16] = {0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1,
			0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
	uint8_t sectorbuffer3[16] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0x78, 0x77,
			0x88, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	// Note 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 must be used for key A
	// for the MAD sector in NDEF records (sector 0)

	// Write block 1 and 2 to the card
	if (!(mifareclassic_WriteDataBlock(1, sectorbuffer1)))
		return 0;
	if (!(mifareclassic_WriteDataBlock(2, sectorbuffer2)))
		return 0;
	// Write key A and access rights card
	if (!(mifareclassic_WriteDataBlock(3, sectorbuffer3)))
		return 0;

	// Seems that everything was OK (?!)
	return 1;
}

uint8_t Pn532::mifareclassic_WriteNDEFURI(uint8_t sectorNumber,
		uint8_t uriIdentifier, const char *url)
{
	// Figure out how long the string is
	uint8_t len = strlen(url);

	// Make sure we're within a 1K limit for the sector number
	if ((sectorNumber < 1) || (sectorNumber > 15))
		return 0;

	// Make sure the URI payload is between 1 and 38 chars
	if ((len < 1) || (len > 38))
		return 0;

	// Note 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 must be used for key A
	// in NDEF records

	// Setup the sector buffer (w/pre-formatted TLV wrapper and NDEF message)
	uint8_t sectorbuffer1[16] = {0x00,
			0x00,
			0x03,
			(uint8_t)(len + 5),
			0xD1,
			0x01,
			(uint8_t)(len + 1),
			0x55,
			uriIdentifier,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00};
	uint8_t sectorbuffer2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t sectorbuffer3[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t sectorbuffer4[16] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7, 0x7F, 0x07,
			0x88, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	if (len <= 6) {
		// Unlikely we'll get a url this short, but why not ...
		memcpy(sectorbuffer1 + 9, url, len);
		sectorbuffer1[len + 9] = 0xFE;
	} else if (len == 7) {
		// 0xFE needs to be wrapped around to next block
		memcpy(sectorbuffer1 + 9, url, len);
		sectorbuffer2[0] = 0xFE;
	} else if ((len > 7) && (len <= 22)) {
		// Url fits in two blocks
		memcpy(sectorbuffer1 + 9, url, 7);
		memcpy(sectorbuffer2, url + 7, len - 7);
		sectorbuffer2[len - 7] = 0xFE;
	} else if (len == 23) {
		// 0xFE needs to be wrapped around to final block
		memcpy(sectorbuffer1 + 9, url, 7);
		memcpy(sectorbuffer2, url + 7, len - 7);
		sectorbuffer3[0] = 0xFE;
	} else {
		// Url fits in three blocks
		memcpy(sectorbuffer1 + 9, url, 7);
		memcpy(sectorbuffer2, url + 7, 16);
		memcpy(sectorbuffer3, url + 23, len - 24);
		sectorbuffer3[len - 22] = 0xFE;
	}

	// Now write all three blocks back to the card
	if (!(mifareclassic_WriteDataBlock(sectorNumber * 4, sectorbuffer1)))
		return 0;
	if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 1, sectorbuffer2)))
		return 0;
	if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 2, sectorbuffer3)))
		return 0;
	if (!(mifareclassic_WriteDataBlock((sectorNumber * 4) + 3, sectorbuffer4)))
		return 0;

	// Seems that everything was OK (?!)
	return 1;
}

uint8_t Pn532::mifareultralight_ReadPage(uint8_t page, uint8_t *buffer)
{
	if (page >= 64) {

		if (_debugger != nullptr)
			_debugger->println("Page value out of range");

		return 0;
	}


	if (_debugger != nullptr)
	{
		_debugger->print("Reading page ");
		_debugger->println(page);
	}

	/* Prepare the command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1;               /* Card number */
	pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
	pn532_packetbuffer[3] = page; /* Page Number (0..63 in most cases) */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {

		if (_debugger != nullptr)
			_debugger->println("Failed to receive ACK for write command");

		return 0;
	}

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	if (_debugger != nullptr)
	{
		_debugger->println("Received: ");
		PrintHexChar(pn532_packetbuffer, 26);
	}

	/* If byte 8 isn't 0x00 we probably have an error */
	if (pn532_packetbuffer[7] == 0x00) {
		/* Copy the 4 data bytes to the output buffer         */
		/* Block content starts at byte 9 of a valid response */
		/* Note that the command actually reads 16 byte or 4  */
		/* pages at a time ... we simply discard the last 12  */
		/* bytes                                              */
		memcpy(buffer, pn532_packetbuffer + 8, 4);
	} else {

		if (_debugger != nullptr)
		{
			_debugger->println("Unexpected response reading block: ");
			PrintHexChar(pn532_packetbuffer, 26);
		}

		return 0;
	}

	/* Display data for debug if requested */

	if (_debugger != nullptr)
	{
		_debugger->print("Page ");
		_debugger->print(page);
		_debugger->println(":");
		PrintHexChar(buffer, 4);
	}

	// Return OK signal
	return 1;
}

uint8_t Pn532::mifareultralight_WritePage(uint8_t page, uint8_t *data)
{
	if (page >= 64) {

		if (_debugger != nullptr)
			_debugger->println("Page value out of range");

		// Return Failed Signal
		return 0;
	}


	if (_debugger != nullptr)
	{
		_debugger->print("Trying to write 4 byte page");
		_debugger->println(page);
	}

	/* Prepare the first command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1; /* Card number */
	pn532_packetbuffer[2] =
			MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
	pn532_packetbuffer[3] = page;    /* Page Number (0..63 for most cases) */
	memcpy(pn532_packetbuffer + 4, data, 4); /* Data Payload */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 8)) {

		if (_debugger != nullptr)
			_debugger->println("Failed to receive ACK for write command");


		// Return Failed Signal
		return 0;
	}
	HAL_Delay(10);

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	// Return OK Signal
	return 1;
}

uint8_t Pn532::ntag2xx_ReadPage(uint8_t page, uint8_t *buffer)
{
	// TAG Type       PAGES   USER START    USER STOP
	// --------       -----   ----------    ---------
	// NTAG 203       42      4             39
	// NTAG 213       45      4             39
	// NTAG 215       135     4             129
	// NTAG 216       231     4             225

	if (page >= 231) {

		if (_debugger != nullptr)
			_debugger->println("Page value out of range");

		return 0;
	}


	if (_debugger != nullptr)
	{
		_debugger->print("Reading page ");
		_debugger->println(page);
	}

	/* Prepare the command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1;               /* Card number */
	pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
	pn532_packetbuffer[3] = page; /* Page Number (0..63 in most cases) */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 4)) {

		if (_debugger != nullptr)
			_debugger->println("Failed to receive ACK for write command");

		return 0;
	}

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	if (_debugger != nullptr)
	{
		_debugger->println("Received: ");
		PrintHexChar(pn532_packetbuffer, 26);
	}

	/* If byte 8 isn't 0x00 we probably have an error */
	if (pn532_packetbuffer[7] == 0x00) {
		/* Copy the 4 data bytes to the output buffer         */
		/* Block content starts at byte 9 of a valid response */
		/* Note that the command actually reads 16 byte or 4  */
		/* pages at a time ... we simply discard the last 12  */
		/* bytes                                              */
		memcpy(buffer, pn532_packetbuffer + 8, 4);
	} else {

		if (_debugger != nullptr)
		{
			_debugger->println("Unexpected response reading block: ");
			PrintHexChar(pn532_packetbuffer, 26);
		}

		return 0;
	}

	/* Display data for debug if requested */

	if (_debugger != nullptr)
	{
		_debugger->print("Page ");
		_debugger->print(page);
		_debugger->println(":");
		PrintHexChar(buffer, 4);
	}

	// Return OK signal
	return 1;
}

uint8_t Pn532::ntag2xx_WritePage(uint8_t page, uint8_t *data)
{
	// TAG Type       PAGES   USER START    USER STOP
	// --------       -----   ----------    ---------
	// NTAG 203       42      4             39
	// NTAG 213       45      4             39
	// NTAG 215       135     4             129
	// NTAG 216       231     4             225

	if ((page < 4) || (page > 225)) {

		if (_debugger != nullptr)
			_debugger->println("Page value out of range");

		// Return Failed Signal
		return 0;
	}


	if (_debugger != nullptr)
	{
		_debugger->print("Trying to write 4 byte page");
		_debugger->println(page);
	}

	/* Prepare the first command */
	pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
	pn532_packetbuffer[1] = 1; /* Card number */
	pn532_packetbuffer[2] =
			MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
	pn532_packetbuffer[3] = page;    /* Page Number (0..63 for most cases) */
	memcpy(pn532_packetbuffer + 4, data, 4); /* Data Payload */

	/* Send the command */
	if (!sendCommandCheckAck(pn532_packetbuffer, 8)) {

		if (_debugger != nullptr)
			_debugger->println("Failed to receive ACK for write command");


		// Return Failed Signal
		return 0;
	}
	HAL_Delay(10);

	/* Read the response packet */
	readdata(pn532_packetbuffer, 26);

	// Return OK Signal
	return 1;
}

uint8_t Pn532::ntag2xx_WriteNDEFURI(uint8_t uriIdentifier, char *url,
		uint8_t dataLen)
{
	uint8_t pageBuffer[4] = {0, 0, 0, 0};

	// Remove NDEF record overhead from the URI data (pageHeader below)
	uint8_t wrapperSize = 12;

	// Figure out how long the string is
	uint8_t len = strlen(url);

	// Make sure the URI payload will fit in dataLen (include 0xFE trailer)
	if ((len < 1) || (len + 1 > (dataLen - wrapperSize)))
		return 0;

	// Setup the record header
	// See NFCForum-TS-Type-2-Tag_1.1.pdf for details
	uint8_t pageHeader[12] = {
			/* NDEF Lock Control TLV (must be first and always present) */
			0x01, /* Tag Field (0x01 = Lock Control TLV) */
			0x03, /* Payload Length (always 3) */
			0xA0, /* The position inside the tag of the lock bytes (upper 4 = page
	               address, lower 4 = byte offset) */
			0x10, /* Size in bits of the lock area */
			0x44, /* Size in bytes of a page and the number of bytes each lock bit can
	               lock (4 bit + 4 bits) */
			/* NDEF Message TLV - URI Record */
			0x03,               /* Tag Field (0x03 = NDEF Message) */
			(uint8_t)(len + 5), /* Payload Length (not including 0xFE trailer) */
			0xD1, /* NDEF Record Header (TNF=0x1:Well known record + SR + ME + MB) */
			0x01, /* Type Length for the record type indicator */
			(uint8_t)(len + 1), /* Payload len */
			0x55,               /* Record Type Indicator (0x55 or 'U' = URI Record) */
			uriIdentifier       /* URI Prefix (ex. 0x01 = "http://www." */
	};

	// Write 12 byte header (three pages of data starting at page 4)
	memcpy(pageBuffer, pageHeader, 4);
	if (!(ntag2xx_WritePage(4, pageBuffer)))
		return 0;
	memcpy(pageBuffer, pageHeader + 4, 4);
	if (!(ntag2xx_WritePage(5, pageBuffer)))
		return 0;
	memcpy(pageBuffer, pageHeader + 8, 4);
	if (!(ntag2xx_WritePage(6, pageBuffer)))
		return 0;

	// Write URI (starting at page 7)
	uint8_t currentPage = 7;
	char *urlcopy = url;
	while (len) {
		if (len < 4) {
			memset(pageBuffer, 0, 4);
			memcpy(pageBuffer, urlcopy, len);
			pageBuffer[len] = 0xFE; // NDEF record footer
			if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
				return 0;
			// DONE!
			return 1;
		} else if (len == 4) {
			memcpy(pageBuffer, urlcopy, len);
			if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
				return 0;
			memset(pageBuffer, 0, 4);
			pageBuffer[0] = 0xFE; // NDEF record footer
			currentPage++;
			if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
				return 0;
			// DONE!
			return 1;
		} else {
			// More than one page of data left
			memcpy(pageBuffer, urlcopy, 4);
			if (!(ntag2xx_WritePage(currentPage, pageBuffer)))
				return 0;
			currentPage++;
			urlcopy += 4;
			len -= 4;
		}
	}

	// Seems that everything was OK (?!)
	return 1;
}

Pn532::~Pn532()
{
	if (_useTwi)
	{
		delete pn532Irq;
		delete pn532Rst;
	}
	else
	{
		delete pn532Cs;
	}
}

void Pn532::i2c_send(uint16_t slaveAddress, uint8_t* buff, uint16_t len)
{
	pn532Twi->write(slaveAddress, buff, len);
}

void Pn532::PrintHex(const uint8_t *data, const uint32_t numBytes)
{
	if (_debugger != nullptr)
	{
		uint32_t szPos;
		for (szPos = 0; szPos < numBytes; szPos++) {

			_debugger->print("0x");
			// Append leading 0 for small values
			if (data[szPos] <= 0xF)
				_debugger->print("0");
			_debugger->print(data[szPos] & 0xff, HEX);
			if ((numBytes > 1) && (szPos != numBytes - 1)) {
				_debugger->print(" ");
			}
		}
		_debugger->println();
	}
}

void Pn532::PrintHexChar(const uint8_t *data, const uint32_t numBytes)
{
	if (_debugger != nullptr)
	{
		uint32_t szPos;
		for (szPos = 0; szPos < numBytes; szPos++) {
			// Append leading 0 for small values
			if (data[szPos] <= 0xF)
				_debugger->print("0");
			_debugger->print(data[szPos], HEX);
			if ((numBytes > 1) && (szPos != numBytes - 1)) {
				_debugger->print(" ");
			}
		}
		_debugger->print("  ");
		for (szPos = 0; szPos < numBytes; szPos++) {
			if (data[szPos] <= 0x1F)
				_debugger->print(".");
			else
				_debugger->print((char)data[szPos]);
		}
		_debugger->println();
	}
}

uint8_t Pn532::i2c_recv(uint16_t slaveAddress)
{
	return ( pn532Twi->read(slaveAddress | 0x01) );
}

void Pn532::readdata(uint8_t *buff, uint8_t n)
{
	if (pn532Spi != nullptr) {
		uint8_t cmd = PN532_SPI_DATAREAD;

		pn532Spi->write_then_read(&cmd, 1, buff, n);

		if (_debugger != nullptr)
		{
			_debugger->print("Reading: ");
			for (uint8_t i = 0; i < n; i++) {
				_debugger->print(" 0x");
				_debugger->print(buff[i], HEX);
			}
			_debugger->println();
		}

	} else {
		// I2C write.

		HAL_Delay(2);


		if (_debugger != nullptr)
			_debugger->print("Reading: ");


		const uint8_t buff_len = n+2;
		uint8_t twiRxBuf[buff_len];
		// Start read (n+1 to take into account leading 0x01 with I2C)
		pn532Twi->read((uint8_t)PN532_I2C_ADDRESS, twiRxBuf, (uint8_t)(n + 2));
		//WIRE.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(n + 2));
		// Discard the leading 0x01
		//i2c_recv();
		for (uint8_t i = 0; i < n; i++) {
			//HAL_Delay(1);
			//buff[i] = i2c_recv();
			buff[i] = twiRxBuf[i+1];

			if (_debugger != nullptr)
			{
				_debugger->print(" 0x");
				_debugger->print(buff[i], HEX);
			}

		}
		// Discard trailing 0x00 0x00
		// i2c_recv();


		if (_debugger != nullptr)
			_debugger->println();

	}
}

void Pn532::writecommand(uint8_t *cmd, uint8_t cmdlen)
{
	const uint8_t cmdLength = cmdlen;

	if (pn532Spi != nullptr) {
		// SPI command write.
		uint8_t checksum;
		uint8_t packet[8 + cmdLength];
		uint8_t *p = packet;
		cmdlen++;

		p[0] = PN532_SPI_DATAWRITE;
		p++;

		p[0] = PN532_PREAMBLE;
		p++;
		p[0] = PN532_STARTCODE1;
		p++;
		p[0] = PN532_STARTCODE2;
		p++;
		checksum = PN532_PREAMBLE + PN532_STARTCODE1 + PN532_STARTCODE2;

		p[0] = cmdlen;
		p++;
		p[0] = ~cmdlen + 1;
		p++;

		p[0] = PN532_HOSTTOPN532;
		p++;
		checksum += PN532_HOSTTOPN532;

		for (uint8_t i = 0; i < cmdlen - 1; i++) {
			p[0] = cmd[i];
			p++;
			checksum += cmd[i];
		}

		p[0] = ~checksum;
		p++;
		p[0] = PN532_POSTAMBLE;
		p++;


		if (_debugger != nullptr)
		{
			_debugger->print("Sending : ");
			for (int i = 1; i < 8 + cmdlen; i++) {
				_debugger->print("0x");
				_debugger->print(packet[i], HEX);
				_debugger->print(", ");
			}
			_debugger->println();

		}
		pn532Spi->write(packet, 8 + cmdlen);

	} else {
		// I2C command write.
		uint8_t checksum;

		cmdlen++;

		if (_debugger != nullptr)
			_debugger->print("\nSending: ");


		HAL_Delay(2); // or whatever the delay is for waking up the board


		// I2C START
		checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;


		uint8_t i2cBuf[cmdlen+7];

		i2cBuf[0] = PN532_PREAMBLE;
		i2cBuf[1] = PN532_PREAMBLE;
		i2cBuf[2] = PN532_STARTCODE2;
		i2cBuf[3] = cmdlen;
		i2cBuf[4] = ~cmdlen + 1;
		i2cBuf[5] = PN532_HOSTTOPN532;

		checksum += PN532_HOSTTOPN532;

		if (_debugger != nullptr)
		{
			_debugger->print(" 0x");
			_debugger->print((uint8_t)PN532_PREAMBLE, HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)PN532_PREAMBLE, HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)PN532_STARTCODE2, HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)cmdlen, HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)(~cmdlen + 1), HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)PN532_HOSTTOPN532, HEX);
		}

		for (uint8_t i = 0; i < cmdlen - 1; i++) {
			i2cBuf[i+6] = cmd[i];
			checksum += cmd[i];

			if (_debugger != nullptr)
			{
				_debugger->print(" 0x");
				_debugger->print((uint8_t)cmd[i], HEX);
			}
		}

		i2cBuf[5+cmdlen] = (uint8_t)~checksum;
		i2cBuf[6+cmdlen] = (uint8_t)PN532_POSTAMBLE;

		i2c_send(PN532_I2C_ADDRESS, i2cBuf, cmdlen+7);

		if (_debugger != nullptr)
		{
			_debugger->print(" 0x");
			_debugger->print((uint8_t)~checksum, HEX);
			_debugger->print(" 0x");
			_debugger->print((uint8_t)PN532_POSTAMBLE, HEX);
			_debugger->println();
		}
	}
}

bool Pn532::isready()
{
	if (pn532Spi != nullptr) {
		uint8_t cmd = PN532_SPI_STATREAD;
		uint8_t reply;
		pn532Spi->write_then_read(&cmd, 1, &reply, 1);
		/*TODO:replace write and read*/

		// Check if status is ready.
		// _debugger->print("Ready? 0x"; _debugger->println(reply, HEX);
		return reply == PN532_SPI_READY;
	} else {
		// I2C check if status is ready by IRQ line being pulled low.
		DigitalStateTypeDef x = pn532Irq->digitalRead();
		return x == DigitalStateTypeDef::LOW;
	}
}

bool Pn532::waitready(uint16_t timeout)
{
	uint16_t timer = 0;
	while (!isready()) {
		if (timeout != 0) {
			timer += 10;
			if (timer > timeout) {

				if (_debugger != nullptr)
					_debugger->println("TIMEOUT!");

				return false;
			}
		}
		HAL_Delay(10);
	}
	return true;
}

bool Pn532::readack()
{
	uint8_t ackbuff[6];

	if (pn532Spi != nullptr) {
		uint8_t cmd = PN532_SPI_DATAREAD;
		pn532Spi->write_then_read(&cmd, 1, ackbuff, 6);
	} else {
		readdata(ackbuff, 6);
	}

	return (0 == memcmp((char *)ackbuff, (char *)pn532ack, 6));
}
