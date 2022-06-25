/*
 * AHT10.cpp
 *
 *  Created on: 6 juin 2022
 *      Author: larbi
 */

#include <AHT10.h>

Twi *aht10SensorTwi = nullptr;

AHT10::AHT10(TwiInstancesTypeDef twiInstance, uint8_t address)
{
	_address = address;
	_twiInstance = twiInstance;
}

DriversExecStatus AHT10::begin()
{
	aht10SensorTwi = Twi::getInstance(_twiInstance);

	if (!aht10SensorTwi->valid())
	{
		Twi::twi_settings_t settings= {
			.clockSpeed= TwiSpeedModeTypeDef::STANDARD_MODE,
			.slaveAddLength= 7,
			.slaveAddress = 0
		};


		if (STATUS_OK != aht10SensorTwi->begin(settings))
		{
			return (STATUS_ERROR);
		}
	}

	// software reset
	if (STATUS_OK != this->softwareReset())
		return (STATUS_ERROR);

	return (STATUS_OK);
}

float AHT10::readTemperature(void)
{
	float temperature = -99.9;

	if (this->readRawData() != STATUS_OK)
		return temperature;

	uint32_t S_temperature = 0;

	S_temperature = ((uint32_t)(_rawDataBuffer[3] & 0x0F) << 16) | ((uint16_t)_rawDataBuffer[4] << 8) | _rawDataBuffer[5];

	temperature = S_temperature * 0.000191  - 50;

	return (temperature);
}

float AHT10::readHumidity(void)
{
	float humidity = -99.9;

	if (this->readRawData() != STATUS_OK)
		return humidity;

	uint32_t S_humidity = 0;

	S_humidity =  (((uint32_t)_rawDataBuffer[1] << 16) | ((uint16_t)_rawDataBuffer[2] << 8) | (_rawDataBuffer[3])) >> 4;

	humidity = S_humidity * 0.000095;

	if (humidity < 0)   return 0;
	if (humidity > 100) return 100;

	return (humidity);
}

DriversExecStatus AHT10::softwareReset()
{
	if (aht10SensorTwi == nullptr)
		return (STATUS_ERROR);

	// software reset
	if (STATUS_OK != aht10SensorTwi->write(_address << 1, AHT10_SOFT_RESET_CMD))
		return (STATUS_ERROR);

	HAL_Delay(1000);

	return (STATUS_OK);
}

AHT10::~AHT10()
{
}

DriversExecStatus AHT10::readRawData(void)
{
	if (aht10SensorTwi == nullptr)
		return (STATUS_ERROR);

	uint8_t initData[3] = {AHT10_TRIGGER_MEASUREMENT_CMD, 0x33, 0x00};

	if (STATUS_OK != aht10SensorTwi->write(_address << 1, initData, 3))
	{
		return (STATUS_ERROR);
	}

	HAL_Delay(300);

	// read data
	if (STATUS_OK != aht10SensorTwi->read( (_address << 1) | 0x01, _rawDataBuffer, 6))
	{
		return (STATUS_ERROR);
	}

	if (_rawDataBuffer[0] == AHT10_ERROR)
		return (STATUS_ERROR);

	return (STATUS_OK);
}
