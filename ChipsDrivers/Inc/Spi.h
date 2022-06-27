/*
 * Spi.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_

#include "ChipsUtil.h"
#include "Gpio.h"

#define SPI_DEFAULT_TIMEOUT 	10000

enum class SpiInstancesTypeDef : uint8_t
{
	SPI_INSTANCE_1,
	SPI_INSTANCE_2,
	SPI_INSTANCE_3
};

enum class SpiDataSizeTypeDef : uint8_t
{
	EIGHT_BITS,
	SIXTEEN_BITS
};

enum class SpiFirstBitTypeDef : uint8_t
{
	LSB,
	MSB
};

enum class SpiClockPrescalerTypeDef : uint8_t
{
	PSC_2,				/*!< 2 */
	PSC_4,				/*!< 4 */
	PSC_8,				/*!< 8 */
	PSC_16,				/*!< 16 */
	PSC_32,				/*!< 32 */
	PSC_64,				/*!< 64 */
	PSC_128,			/*!< 128 */
	PSC_256				/*!< 256 */
};

enum class SpiClockModeTypeDef : uint8_t
{
	MODE_0,			/*!< CPOL = 0, CPHA = 0 */
	MODE_1,			/*!< CPOL = 0, CPHA = 1 */
	MODE_2,			/*!< CPOL = 1, CPHA = 0 */
	MODE_3			/*!< CPOL = 1, CPHA = 1 */
};


class Spi {
public:
	static Spi *getInstance(SpiInstancesTypeDef channel);
	static void deleteInstance(SpiInstancesTypeDef channel);

	using spi_settings_t = struct {
		SpiDataSizeTypeDef dataSize:2;
		SpiFirstBitTypeDef firstBit:2;
		SpiClockPrescalerTypeDef clockPrescaler:3;
		SpiClockModeTypeDef clockMode:2;
	};

	SpiInstancesTypeDef getChannel() const
	{
		return _channel;
	}

	const Gpio* getCsPin() const
	{
		return _csPin;
	}

	const spi_settings_t& getSettings() const
	{
		return _settings;
	}

	void* get_hspi() { return _hspi; }

	bool valid(){ return _valid;}

	DriversExecStatus begin(spi_settings_t setting, Gpio* csPin);

	bool read(uint8_t *buffer, uint16_t len, uint8_t sendvalue = 0xFF, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	bool write(const uint8_t *buffer, uint16_t len,
			const uint8_t *prefix_buffer = nullptr, uint16_t prefix_len = 0, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	bool write_then_read(const uint8_t *write_buffer, uint16_t write_len,
			uint8_t *read_buffer, size_t read_len,
			uint8_t sendvalue = 0xFF, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	bool write_and_read(uint8_t *buffer, uint16_t len, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	uint8_t transfer(uint8_t send, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	DriversExecStatus transfer(uint8_t *buffer, uint16_t len, uint32_t timeout = SPI_DEFAULT_TIMEOUT);

	DriversExecStatus end(void);

	static Spi* spi_channel[3];

private:

	Spi(SpiInstancesTypeDef channel);
	virtual ~Spi();

	void* _hspi{nullptr};

	SpiInstancesTypeDef _channel;

	spi_settings_t _settings;

	bool _valid{false};

	Gpio* _csPin = nullptr;

	void setChipSelect(DigitalStateTypeDef state);
};

#endif /* INC_SPI_H_ */
