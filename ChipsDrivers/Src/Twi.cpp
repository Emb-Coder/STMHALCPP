/*
 * Twi.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Twi.h>

Twi *Twi::twi_channel[2] = {nullptr, nullptr};

Twi* Twi::getInstance(TwiInstancesTypeDef channel)
{
    if(channel > TwiInstancesTypeDef::I2C_3 || (channel < TwiInstancesTypeDef::I2C_1)) return nullptr;
    if(twi_channel[(uint8_t)channel] == nullptr) {
        twi_channel[(uint8_t)channel] = new Twi(channel);
    }

    return twi_channel[(uint8_t)channel];
}

void Twi::deleteInstance(TwiInstancesTypeDef channel)
{
    if(channel > TwiInstancesTypeDef::I2C_3 || (channel < TwiInstancesTypeDef::I2C_1)) return;
    if(twi_channel[(uint8_t)channel] == nullptr) {
        return;
    }
    delete twi_channel[(uint8_t)channel];
    twi_channel[(uint8_t)channel] = nullptr;
}

DriversExecStatus Twi::begin(twi_settings_t setting)
{
	if (_channel < TwiInstancesTypeDef::I2C_1 || _channel > TwiInstancesTypeDef::I2C_3 ||
		setting.clockSpeed < TwiSpeedModeTypeDef::STANDARD_MODE || setting.clockSpeed > TwiSpeedModeTypeDef::FAST_MODE ||
		(setting.slaveAddLength != 7 && setting.slaveAddLength != 10) )
	{
		return (STATUS_ERROR);
	}

	_settings = setting;

	I2C_HandleTypeDef i2cHandle = {0};

	i2cHandle.Instance = _channel == TwiInstancesTypeDef::I2C_1 ? I2C1:
									 I2C3;

	// standard mode (100khz, fast mode 400khz, fast mode plus 1000khz (not implemented)
	i2cHandle.Init.ClockSpeed = setting.clockSpeed == TwiSpeedModeTypeDef::STANDARD_MODE ? 100000 :
								400000;

	i2cHandle.Init.DutyCycle = I2C_DUTYCYCLE_2;
	i2cHandle.Init.OwnAddress1 = setting.slaveAddress;
	i2cHandle.Init.AddressingMode = setting.slaveAddLength == 7 ? I2C_ADDRESSINGMODE_7BIT:
									I2C_ADDRESSINGMODE_10BIT;
	i2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2cHandle.Init.OwnAddress2 = 0;
	i2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2cHandle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&i2cHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

    _hi2c = malloc(sizeof(I2C_HandleTypeDef));
    memcpy(_hi2c, &i2cHandle, sizeof(I2C_HandleTypeDef));
    _valid = true;

	return (STATUS_OK);

}

extern "C" void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(hi2c->Instance==I2C1)
	{

		if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
			__HAL_RCC_GPIOB_CLK_ENABLE();


//		/**I2C1 GPIO Configuration
//		PB6     ------> I2C1_SCL
//		PB7     ------> I2C1_SDA
//		 */
//
//		GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
//		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//		GPIO_InitStruct.Pull = GPIO_NOPULL;
//		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//		GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
//		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	    /**I2C1 GPIO Configuration
	    PB8     ------> I2C1_SCL
	    PB9     ------> I2C1_SDA
	    */
	    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
	    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	    GPIO_InitStruct.Pull = GPIO_NOPULL;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* Peripheral clock enable */
		__HAL_RCC_I2C1_CLK_ENABLE();
	}
	else if(hi2c->Instance==I2C3)
	{

		if (__HAL_RCC_GPIOC_IS_CLK_DISABLED())
			__HAL_RCC_GPIOC_CLK_ENABLE();

		if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			__HAL_RCC_GPIOA_CLK_ENABLE();


		/**I2C3 GPIO Configuration
		PC9     ------> I2C3_SDA
		PA8     ------> I2C3_SCL
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* Peripheral clock enable */
		__HAL_RCC_I2C3_CLK_ENABLE();

	}

}

DriversExecStatus Twi::write(uint16_t slaveAddress, uint8_t* data,
		uint16_t dataLen, uint32_t timeout)
{

	if (HAL_OK != HAL_I2C_Master_Transmit((I2C_HandleTypeDef*)_hi2c, slaveAddress, data, dataLen, timeout))
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

DriversExecStatus Twi::write(uint16_t slaveAddress, uint8_t val,
		uint32_t timeout)
{
	return (this->write(slaveAddress, &val, 1, timeout));
}

uint8_t Twi::read(uint16_t slaveAddress, uint32_t timeout)
{
	uint8_t rxValue = 0;

	this->read(slaveAddress, &rxValue, 1, timeout);

	return (rxValue);
}

DriversExecStatus Twi::read(uint16_t slaveAddress, uint8_t *RxDataBuf,
		uint16_t DataLength, uint32_t timeout)
{
	if (HAL_OK != HAL_I2C_Master_Receive((I2C_HandleTypeDef*)_hi2c, slaveAddress, RxDataBuf, DataLength, timeout))
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}


DriversExecStatus Twi::regWrite(uint16_t slaveAddress, uint16_t regAddress, uint8_t regAddSize,
		uint8_t *data, uint16_t dataLen, uint32_t timeout)
{
	if (HAL_OK !=  HAL_I2C_Mem_Write((I2C_HandleTypeDef*)_hi2c, slaveAddress, regAddress, regAddSize, data, dataLen, timeout))
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

DriversExecStatus Twi::regRead(uint16_t slaveAddress, uint16_t regAddress, uint8_t regAddSize,
		uint8_t *rxData, uint16_t rxDataLen, uint32_t timeout)
{
	if (HAL_OK != HAL_I2C_Mem_Read((I2C_HandleTypeDef*)_hi2c, slaveAddress, regAddress, regAddSize, rxData, rxDataLen, timeout))
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

void Twi::scan(uint8_t *slavesAddBuf, uint32_t timeout)
{
	uint8_t slaveCounter = 0;

	for(uint8_t address = 0x01; address < 0xFF; address++)
	{
		if (HAL_OK == HAL_I2C_IsDeviceReady((I2C_HandleTypeDef*)_hi2c, address, 10, timeout))
		{
			slavesAddBuf[slaveCounter] = address;
		}
	}
}


DriversExecStatus Twi::end(void)
{
    HAL_I2C_DeInit((I2C_HandleTypeDef*)_hi2c);
	return (STATUS_OK);
}

extern "C" void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
	if(hi2c->Instance==I2C1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();

		/**I2C1 GPIO Configuration
	    PB6     ------> I2C1_SCL
	    PB7     ------> I2C1_SDA
		 */
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
	}
	else if(hi2c->Instance==I2C3)
	{
		/* Peripheral clock disable */
		__HAL_RCC_I2C3_CLK_DISABLE();

		/**I2C3 GPIO Configuration
	    PC9     ------> I2C3_SDA
	    PA8     ------> I2C3_SCL
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
	}
}

Twi::Twi(TwiInstancesTypeDef channel)
{
    _valid = false;
    _channel = channel;
}


Twi::~Twi() {
    if(_valid) {
        HAL_I2C_DeInit((I2C_HandleTypeDef*)_hi2c);
        free(_hi2c);

        // TODO : delete buffer or clear
    }
}

