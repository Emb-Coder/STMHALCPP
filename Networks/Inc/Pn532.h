/*
 * Pn532.h
 *
 *  Created on: 17 juin 2022
 *      Author: larbi
 */

#ifndef NETWORKS_INC_PN532_H_
#define NETWORKS_INC_PN532_H_

#include "Twi.h"
#include "Spi.h"
#include "Gpio.h"

#include "Serial.h"

#include "Pn532Cmd.h"

#define PN532_DEFAULT_TIMEOUT	10000

class Pn532 {
public:

	Pn532(SpiInstancesTypeDef spiInstance, std::pair<GpioPortsTypeDef, uint16_t> ssPin);
	Pn532(TwiInstancesTypeDef twiInstance, std::pair<GpioPortsTypeDef, uint16_t> irqPin, std::pair<GpioPortsTypeDef, uint16_t> rstPin);

	DriversExecStatus begin(void);

	void setDebugger(Serial *debugger) {
		this->_debugger = debugger;
	}

	// Generic PN532 functions
	bool SAMConfig(void);
	uint32_t getFirmwareVersion(void);
	bool sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen,
			uint16_t timeout = 100);
	bool writeGPIO(uint8_t pinstate);
	uint8_t readGPIO(void);
	bool setPassiveActivationRetries(uint8_t maxRetries);

	// ISO14443A functions
	bool readPassiveTargetID(
			uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength,
			uint16_t timeout = 0); // timeout 0 means no timeout - will block forever.

	bool startPassiveTargetIDDetection(uint8_t cardbaudrate);
	bool readDetectedPassiveTargetID(uint8_t *uid, uint8_t *uidLength);

	bool inDataExchange(uint8_t *send, uint8_t sendLength, uint8_t *response,
			uint8_t *responseLength);

	bool inListPassiveTarget();
	uint8_t AsTarget();
	uint8_t getDataTarget(uint8_t *cmd, uint8_t *cmdlen);
	uint8_t setDataTarget(uint8_t *cmd, uint8_t cmdlen);

	// Mifare Classic functions
	bool mifareclassic_IsFirstBlock(uint32_t uiBlock);
	bool mifareclassic_IsTrailerBlock(uint32_t uiBlock);

	uint8_t mifareclassic_AuthenticateBlock(uint8_t *uid, uint8_t uidLen,
			uint32_t blockNumber,
			uint8_t keyNumber, uint8_t *keyData);

	uint8_t mifareclassic_ReadDataBlock(uint8_t blockNumber, uint8_t *data);
	uint8_t mifareclassic_WriteDataBlock(uint8_t blockNumber, uint8_t *data);
	uint8_t mifareclassic_FormatNDEF(void);

	uint8_t mifareclassic_WriteNDEFURI(uint8_t sectorNumber,
			uint8_t uriIdentifier, const char *url);

	// Mifare Ultralight functions
	uint8_t mifareultralight_ReadPage(uint8_t page, uint8_t *buffer);
	uint8_t mifareultralight_WritePage(uint8_t page, uint8_t *data);

	// NTAG2xx functions
	uint8_t ntag2xx_ReadPage(uint8_t page, uint8_t *buffer);
	uint8_t ntag2xx_WritePage(uint8_t page, uint8_t *data);

	uint8_t ntag2xx_WriteNDEFURI(uint8_t uriIdentifier, char *url,
			uint8_t dataLen);

	virtual ~Pn532();

	const Serial* getDebugger() const {
		return _debugger;
	}

	const std::pair<GpioPortsTypeDef, uint16_t>& getIrqPin() const {
		return _irqPin;
	}

	const std::pair<GpioPortsTypeDef, uint16_t>& getRstPin() const {
		return _rstPin;
	}

	SpiInstancesTypeDef getSpiInstance() const {
		return _spiInstance;
	}

	const std::pair<GpioPortsTypeDef, uint16_t>& getSsPin() const {
		return _ssPin;
	}

	uint32_t getTimeout() const {
		return _timeout;
	}

	void setTimeout(uint32_t timeout) {
		_timeout = timeout;
	}

	TwiInstancesTypeDef getTwiInstance() const {
		return _twiInstance;
	}

	bool isUseTwi() const {
		return _useTwi;
	}

	const Gpio* getPn532Cs() const {
		return pn532Cs;
	}

	const Gpio* getPn532Irq() const {
		return pn532Irq;
	}

	const Gpio* getPn532Rst() const {
		return pn532Rst;
	}

	const Spi* getPn532Spi() const {
		return pn532Spi;
	}

	const Twi* getPn532Twi() const {
		return pn532Twi;
	}

private:
	uint32_t _timeout;

	SpiInstancesTypeDef _spiInstance;
	std::pair<GpioPortsTypeDef, uint16_t> _ssPin;

	TwiInstancesTypeDef _twiInstance;
	std::pair<GpioPortsTypeDef, uint16_t> _rstPin;
	std::pair<GpioPortsTypeDef, uint16_t> _irqPin;

	bool _useTwi = false;

	Serial* _debugger = nullptr;

	int8_t _uid[7];      // ISO14443A uid
	int8_t _uidLen;      // uid len
	int8_t _key[6];      // Mifare Classic key
	int8_t _inListedTag; // Tg number of inlisted tag.

	// Low level communication functions that handle both SPI and I2C.
	void readdata(uint8_t *buff, uint8_t n);
	void writecommand(uint8_t *cmd, uint8_t cmdlen);
	bool isready();
	bool waitready(uint16_t timeout);
	bool readack();

	void i2c_send(uint16_t slaveAddress, uint8_t* buff, uint16_t len);
	uint8_t i2c_recv(uint16_t slaveAddress);

	// Help functions to display formatted text
	void PrintHex(const uint8_t *data, const uint32_t numBytes);
	void PrintHexChar(const uint8_t *pbtData, const uint32_t numBytes);


	Spi* pn532Spi = nullptr;
	Twi* pn532Twi = nullptr;

	Gpio* pn532Cs = nullptr;
	Gpio* pn532Irq = nullptr;
	Gpio* pn532Rst = nullptr;

};

#endif /* NETWORKS_INC_PN532_H_ */
