/*
 * AHT10.h
 *
 *  Created on: 6 juin 2022
 *      Author: larbi
 */

#ifndef SENSORS_INC_AHT10_H_
#define SENSORS_INC_AHT10_H_

#include "Twi.h"

#define AHT10_DEFAULT_ADDRESS			0x38

#define AHT10_ERROR						0xFF

#define AHT10_SOFT_RESET_CMD			0xBA
#define AHT10_TRIGGER_MEASUREMENT_CMD 	0xAC


class AHT10 {
public:
	AHT10(TwiInstancesTypeDef twiInstance, uint8_t address = AHT10_DEFAULT_ADDRESS);

	DriversExecStatus begin();

	float readTemperature(void);
	float readHumidity(void);

	DriversExecStatus softwareReset();

	virtual ~AHT10();
private:
	uint8_t _address;
	TwiInstancesTypeDef _twiInstance;

	uint8_t _rawDataBuffer[6] = {AHT10_ERROR, 0, 0, 0, 0, 0};

	DriversExecStatus readRawData(void);
};

#endif /* SENSORS_INC_AHT10_H_ */
