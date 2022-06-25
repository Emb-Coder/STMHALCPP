/*
 * Spi.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Spi.h>

Spi *Spi::spi_channel[3] = {nullptr, nullptr, nullptr};

Spi* Spi::getInstance(SpiInstancesTypeDef channel)
{
	if(channel > SpiInstancesTypeDef::SPI_INSTANCE_3 || (channel < SpiInstancesTypeDef::SPI_INSTANCE_1)) return nullptr;

	if(spi_channel[(uint8_t)channel] == nullptr) {
		spi_channel[(uint8_t)channel] = new Spi(channel);
	}

	return spi_channel[(uint8_t)channel];
}

void Spi::deleteInstance(SpiInstancesTypeDef channel)
{
    if(channel > SpiInstancesTypeDef::SPI_INSTANCE_3 || (channel < SpiInstancesTypeDef::SPI_INSTANCE_1)) return;
    if(spi_channel[(uint8_t)channel] == nullptr) {
        return;
    }
    delete spi_channel[(uint8_t)channel];
    spi_channel[(uint8_t)channel] = nullptr;
}

DriversExecStatus Spi::begin(spi_settings_t setting, Gpio* csPin)
{
	if (_channel < SpiInstancesTypeDef::SPI_INSTANCE_1 || _channel > SpiInstancesTypeDef::SPI_INSTANCE_3 ||
		setting.dataSize < SpiDataSizeTypeDef::EIGHT_BITS || setting.dataSize > SpiDataSizeTypeDef::SIXTEEN_BITS ||
		setting.firstBit < SpiFirstBitTypeDef::LSB || setting.firstBit > SpiFirstBitTypeDef::MSB ||
		setting.clockPrescaler < SpiClockPrescalerTypeDef::PSC_2 || setting.clockPrescaler > SpiClockPrescalerTypeDef::PSC_256 ||
		setting.clockMode < SpiClockModeTypeDef::MODE_0 || setting.clockMode > SpiClockModeTypeDef::MODE_3 )
	{
		return (STATUS_ERROR);
	}

	_csPin = csPin;
	_settings = setting;

	setChipSelect(DigitalStateTypeDef::HIGH);

	SPI_HandleTypeDef spiHandle = {0};

	spiHandle.Instance = _channel == SpiInstancesTypeDef::SPI_INSTANCE_1 ? SPI1:
						 _channel == SpiInstancesTypeDef::SPI_INSTANCE_2 ? SPI2:
						 SPI3;

	spiHandle.Init.Mode = SPI_MODE_MASTER;
	spiHandle.Init.Direction = SPI_DIRECTION_2LINES;

	spiHandle.Init.DataSize = setting.dataSize == SpiDataSizeTypeDef::EIGHT_BITS ? SPI_DATASIZE_8BIT:
							  SPI_DATASIZE_16BIT;

	spiHandle.Init.CLKPolarity = setting.clockMode == SpiClockModeTypeDef::MODE_0 || setting.clockMode == SpiClockModeTypeDef::MODE_1 ? SPI_POLARITY_LOW:
								 SPI_POLARITY_HIGH;

	spiHandle.Init.CLKPhase = setting.clockMode == SpiClockModeTypeDef::MODE_0 || setting.clockMode == SpiClockModeTypeDef::MODE_2 ? SPI_PHASE_1EDGE:
							  SPI_PHASE_2EDGE;

	spiHandle.Init.NSS = SPI_NSS_SOFT;
	spiHandle.Init.BaudRatePrescaler = setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_2 ? SPI_BAUDRATEPRESCALER_2:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_4 ? SPI_BAUDRATEPRESCALER_4:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_8 ? SPI_BAUDRATEPRESCALER_8:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_16 ? SPI_BAUDRATEPRESCALER_16:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_32 ? SPI_BAUDRATEPRESCALER_32:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_64 ? SPI_BAUDRATEPRESCALER_64:
									   setting.clockPrescaler == SpiClockPrescalerTypeDef::PSC_128 ? SPI_BAUDRATEPRESCALER_128:
									   SPI_BAUDRATEPRESCALER_256;

	spiHandle.Init.FirstBit = setting.firstBit == SpiFirstBitTypeDef::MSB ? SPI_FIRSTBIT_MSB:
							  SPI_FIRSTBIT_LSB;

	spiHandle.Init.TIMode = SPI_TIMODE_DISABLE;
	spiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	spiHandle.Init.CRCPolynomial = 10;

	if (HAL_SPI_Init(&spiHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

    _hspi = malloc(sizeof(SPI_HandleTypeDef));
    memcpy(_hspi, &spiHandle, sizeof(SPI_HandleTypeDef));
    _valid = true;

    return (STATUS_OK);
}


extern "C" void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(hspi->Instance==SPI1)
	{
		/* Peripheral clock enable */
		__HAL_RCC_SPI1_CLK_ENABLE();

		if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			__HAL_RCC_GPIOA_CLK_ENABLE();

		/**SPI1 GPIO Configuration
		PA5     ------> SPI1_SCK
		PA6     ------> SPI1_MISO
		PA7     ------> SPI1_MOSI
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	}
	else if(hspi->Instance==SPI2)
	{
		/* Peripheral clock enable */
		__HAL_RCC_SPI2_CLK_ENABLE();

		if (__HAL_RCC_GPIOC_IS_CLK_DISABLED())
			__HAL_RCC_GPIOC_CLK_ENABLE();

		if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
			__HAL_RCC_GPIOB_CLK_ENABLE();

		/**SPI2 GPIO Configuration
		PC2     ------> SPI2_MISO
		PC3     ------> SPI2_MOSI
		PB10     ------> SPI2_SCK
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	else if(hspi->Instance==SPI3)
	{

		/* Peripheral clock enable */
		__HAL_RCC_SPI3_CLK_ENABLE();

		if (__HAL_RCC_GPIOC_IS_CLK_DISABLED())
			__HAL_RCC_GPIOC_CLK_ENABLE();

		/**SPI3 GPIO Configuration
		PC10     ------> SPI3_SCK
		PC11     ------> SPI3_MISO
		PC12     ------> SPI3_MOSI
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	}

}

DriversExecStatus Spi::end(void)
{
	HAL_SPI_DeInit((SPI_HandleTypeDef*)_hspi);
	_csPin = nullptr;

	return (STATUS_OK);
}

extern "C" void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
	if(hspi->Instance==SPI1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_SPI1_CLK_DISABLE();

		/**SPI1 GPIO Configuration
		PA5     ------> SPI1_SCK
		PA6     ------> SPI1_MISO
		PA7     ------> SPI1_MOSI
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
	}
	else if(hspi->Instance==SPI2)
	{
		/* Peripheral clock disable */
		__HAL_RCC_SPI2_CLK_DISABLE();

		/**SPI2 GPIO Configuration
		PC2     ------> SPI2_MISO
		PC3     ------> SPI2_MOSI
		PB10     ------> SPI2_SCK
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2|GPIO_PIN_3);

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

	}
	else if(hspi->Instance==SPI3)
	{
		/* Peripheral clock disable */
		__HAL_RCC_SPI3_CLK_DISABLE();

		/**SPI3 GPIO Configuration
		PC10     ------> SPI3_SCK
		PC11     ------> SPI3_MISO
		PC12     ------> SPI3_MOSI
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);

	}

}


Spi::Spi(SpiInstancesTypeDef channel)
{
    _valid = false;
    _channel = channel;
}

Spi::~Spi()
{
	_csPin = nullptr;

	if(_valid) {
		HAL_SPI_DeInit((SPI_HandleTypeDef*)_hspi);
		free(_hspi);

		// TODO : delete buffer or clear
	}
}

bool Spi::read(uint8_t *buffer, uint16_t len, uint8_t sendvalue, uint32_t timeout)
{
	memset(buffer, sendvalue, len); // clear out existing buffer

	setChipSelect(LOW);

	DriversExecStatus ret = this->transfer(buffer, len, timeout);

	setChipSelect(HIGH);

	if (ret == this->transfer(buffer, len, timeout))
		return (true);
	else
		return (false);
}

bool Spi::write(const uint8_t *buffer, uint16_t len, const uint8_t *prefix_buffer,
		uint16_t prefix_len, uint32_t timeout)
{
	setChipSelect(LOW);

    for (uint16_t i = 0; i < prefix_len; i++) {
    	this->transfer(prefix_buffer[i], timeout);
    }
    for (uint16_t i = 0; i < len; i++) {
    	this->transfer(buffer[i], timeout);
    }

	setChipSelect(HIGH);

    return true;
}

bool Spi::write_then_read(const uint8_t *write_buffer, uint16_t write_len,
		uint8_t *read_buffer, size_t read_len,
		uint8_t sendvalue, uint32_t timeout)
{
	setChipSelect(LOW);

	for (size_t i = 0; i < write_len; i++) {
		transfer(write_buffer[i], timeout);
	}

	// do the reading
	for (size_t i = 0; i < read_len; i++) {
		read_buffer[i] = transfer(sendvalue, timeout);
	}

	setChipSelect(HIGH);

	return (true);
}

bool Spi::write_and_read(uint8_t *buffer, uint16_t len, uint32_t timeout)
{
	setChipSelect(LOW);

	transfer(buffer, len, timeout);

	setChipSelect(HIGH);

	return (true);
}

uint8_t Spi::transfer(uint8_t send, uint32_t timeout)
{
	uint8_t data = send;

	this->transfer(&data, 1, timeout);

	return (data);
}

DriversExecStatus Spi::transfer(uint8_t *buffer, uint16_t len, uint32_t timeout)
{
	if (HAL_OK != HAL_SPI_TransmitReceive((SPI_HandleTypeDef*)_hspi, buffer, buffer, len, timeout))
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}


void Spi::setChipSelect(DigitalStateTypeDef state)
{
	if (_csPin != nullptr)
		_csPin->digitalWrite(state);
}
