/*
 * Serial.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_SERIAL_H_
#define INC_SERIAL_H_

#include "ChipsUtil.h"

#define SERIAL_DEFAULT_TIMEOUT 	10000

#define DEC 					10
#define HEX 					16
#define OCT 					8
#define BIN 					2

#define RX_BFR_SIZE   			510
#define MAIN_BFR_SIZE 			255

enum class SerialInstancesTypeDef : uint8_t
{
	SERIAL_1,
	SERIAL_2,
	SERIAL_6
};

enum class SerialWordLengthTypeDef : uint8_t
{
	EIGHT_BITS_LENGTH,
	NINE_BITS_LENGTH
};

enum class SerialStopBitTypeDef : uint8_t
{
	ONE_STOP_BIT,
	TWO_STOP_BITS
};

enum class SerialParityTypeDef : uint8_t
{
	NONE,
	EVEN,
	ODD
};

class Serial {

public:
	static Serial *getInstance(SerialInstancesTypeDef channel);
	static void deleteInstance(SerialInstancesTypeDef channel);

	using settings_t = struct __packed {
        uint32_t baudrate:24;
        SerialWordLengthTypeDef  wordLength:2;
        SerialStopBitTypeDef  stopBits:2;
        SerialParityTypeDef  parity:2;
	};

	SerialInstancesTypeDef getChannel() const
	{
		return _channel;
	}

	const uint8_t* getMainBuffer() const
	{
		return _mainBuffer;
	}

	const settings_t& getSettings() const
	{
		return _settings;
	}

	void* get_huart() { return _huart; }

	bool valid(){ return _valid;}

	DriversExecStatus begin(settings_t setting);
	DriversExecStatus end(void);

	void flush();

	DriversExecStatus write(uint8_t val, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus write(uint8_t* dataBuffer, uint16_t length, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);

	uint16_t read(uint8_t* dataBuffer, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	uint16_t read(char* dataBuffer, uint32_t timeout);

	DriversExecStatus readByte(uint8_t* val, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus readData(uint8_t* dataBuffer, uint16_t size, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);

	DriversExecStatus readBytesUntil(uint8_t* dataBuffer, const uint8_t* delimiter, uint16_t sizeOfDelim, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus readStringUntil(char* RxData, const char* delimiter, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);

	DriversExecStatus printf(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));

	DriversExecStatus print(const std::string &s, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(const char* str, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(char c, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(unsigned char uc, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(int val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(unsigned int val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(unsigned long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(long long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(unsigned long long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(double number, int digits = 2, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus print(struct tm * timeinfo, const char * format = NULL, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);


	DriversExecStatus println(const std::string &s, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(const char* str, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(char c, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(unsigned char uc, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(int val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(unsigned int val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(unsigned long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(long long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(unsigned long long val, int = DEC, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(double val, int digitsAfterDecimalP = 2, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);
	DriversExecStatus println(struct tm * timeinfo, const char * format = NULL, uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);

	DriversExecStatus println(uint32_t timeout = SERIAL_DEFAULT_TIMEOUT);

	void processCallback(bool state);

	static Serial* serial_channel[3];
private:
	Serial(SerialInstancesTypeDef channel);
	virtual ~Serial();

	void *_huart{nullptr};
	SerialInstancesTypeDef _channel;
    settings_t _settings;
	bool _valid{false};

	uint8_t _rxRollover = 0;
	uint8_t _rxCounter = 0;
	uint16_t _rxBfrPos = 0;
	uint8_t _mainBufferCounter = 0;

	uint8_t _mainBuffer[MAIN_BFR_SIZE];

	volatile bool _rxDataFlag = false;
	uint16_t _rxDataLen = 0;
};

#endif /* INC_SERIAL_H_ */
