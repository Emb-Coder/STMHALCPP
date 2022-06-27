/*
 * Twi.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_TWI_H_
#define INC_TWI_H_

#include "ChipsUtil.h"

#define TWI_DEFAULT_TIMEOUT 	10000

/*
 * \note : I2C2_SDA pin is shared with trace asynchronous SWD pin
 *
 * 	We won't use this instance for this version
 *
 */
enum class TwiInstancesTypeDef : uint8_t
{
	I2C_1,
	I2C_3
};

enum class TwiSpeedModeTypeDef : uint8_t
{
	STANDARD_MODE,
	FAST_MODE
};

class Twi {
public:
	static Twi *getInstance(TwiInstancesTypeDef channel);
	static void deleteInstance(TwiInstancesTypeDef channel);

	using twi_settings_t = struct __packed {
		TwiSpeedModeTypeDef clockSpeed:2;
        uint8_t  slaveAddLength:4;
        uint32_t  slaveAddress;
	};

	TwiInstancesTypeDef getChannel() const
	{
		return _channel;
	}

	const twi_settings_t& getSettings() const
	{
		return _settings;
	}

	void* get_hi2c() { return _hi2c; }

	bool valid(){ return _valid;}

	DriversExecStatus begin(twi_settings_t setting);

	DriversExecStatus write(uint16_t slaveAddress, uint8_t* data, uint16_t dataLen, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	DriversExecStatus write(uint16_t slaveAddress, uint8_t data, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	uint8_t read(uint16_t slaveAddress, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	DriversExecStatus read(uint16_t slaveAddress, uint8_t* RxDataBuf, uint16_t DataLength, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	DriversExecStatus regWrite(uint16_t slaveAddress, uint16_t regAddress, uint8_t regAddSize, uint8_t *data, uint16_t dataLen, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	DriversExecStatus regRead(uint16_t slaveAddress, uint16_t regAddress, uint8_t regAddSize, uint8_t *rxData, uint16_t rxDataLen, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	void scan(uint8_t* slavesAddBuf, uint32_t timeout = TWI_DEFAULT_TIMEOUT);

	DriversExecStatus end(void);

	static Twi* twi_channel[2];

private:

	Twi(TwiInstancesTypeDef channel);
	virtual ~Twi();

	void* _hi2c{nullptr};

	TwiInstancesTypeDef _channel;
	twi_settings_t _settings;
	bool _valid{false};

};

#endif /* INC_TWI_H_ */
